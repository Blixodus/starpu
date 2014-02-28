/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2009-2014  Université de Bordeaux 1
 * Copyright (C) 2010, 2011, 2012, 2013, 2014  Centre National de la Recherche Scientifique
 *
 * StarPU is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * StarPU is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU Lesser General Public License in COPYING.LGPL for more details.
 */

#include <stdint.h>

#include <datawizard/datawizard.h>
#include <core/dependencies/data_concurrency.h>
#include <common/uthash.h>
#include <common/starpu_spinlock.h>
#include <core/task.h>
#include <core/workers.h>
#include <datawizard/memstats.h>

/* Entry in the `registered_handles' hash table.  */
struct handle_entry
{
	UT_hash_handle hh;
	void *pointer;
	starpu_data_handle_t handle;
};

/* Hash table mapping host pointers to data handles.  */
static struct handle_entry *registered_handles;
static struct _starpu_spinlock    registered_handles_lock;
static int _data_interface_number = STARPU_MAX_INTERFACE_ID;

/* Entry in the `registered_tag_handles' hash table.  */
struct handle_tag_entry
{
	UT_hash_handle hh;
	int tag;
	starpu_data_handle_t handle;
};

/* Hash table mapping host tags to data handles.  */
static struct handle_tag_entry *registered_tag_handles;
static struct _starpu_spinlock    registered_tag_handles_lock;

void _starpu_data_interface_init(void)
{
	_starpu_spin_init(&registered_handles_lock);
	_starpu_spin_init(&registered_tag_handles_lock);
}

void _starpu_data_interface_shutdown()
{
	struct handle_entry *entry, *tmp;

	_starpu_spin_destroy(&registered_handles_lock);

	HASH_ITER(hh, registered_handles, entry, tmp)
	{
		HASH_DEL(registered_handles, entry);
		free(entry);
	}

	registered_handles = NULL;

	struct handle_tag_entry *tag_entry, *tag_tmp;

	_starpu_spin_destroy(&registered_tag_handles_lock);

	HASH_ITER(hh, registered_tag_handles, tag_entry, tag_tmp)
	{
		HASH_DEL(registered_tag_handles, tag_entry);
		free(tag_entry);
	}

	registered_tag_handles = NULL;
}

struct starpu_data_interface_ops *_starpu_data_interface_get_ops(unsigned interface_id)
{
	switch (interface_id)
	{
		case STARPU_MATRIX_INTERFACE_ID:
			return &starpu_interface_matrix_ops;

		case STARPU_BLOCK_INTERFACE_ID:
			return &starpu_interface_block_ops;

		case STARPU_VECTOR_INTERFACE_ID:
			return &starpu_interface_vector_ops;

		case STARPU_CSR_INTERFACE_ID:
			return &starpu_interface_csr_ops;

		case STARPU_BCSR_INTERFACE_ID:
			return &starpu_interface_bcsr_ops;

		case STARPU_VARIABLE_INTERFACE_ID:
			return &starpu_interface_variable_ops;

		case STARPU_VOID_INTERFACE_ID:
			return &starpu_interface_void_ops;

		case STARPU_MULTIFORMAT_INTERFACE_ID:
			return &starpu_interface_multiformat_ops;

		default:
			STARPU_ABORT();
			return NULL;
	}
}

/* Register the mapping from PTR to HANDLE.  If PTR is already mapped to
 * some handle, the new mapping shadows the previous one.   */
void _starpu_data_register_ram_pointer(starpu_data_handle_t handle, void *ptr)
{
	struct handle_entry *entry;

	entry = (struct handle_entry *) malloc(sizeof(*entry));
	STARPU_ASSERT(entry != NULL);

	entry->pointer = ptr;
	entry->handle = handle;

	_starpu_spin_lock(&registered_handles_lock);
	HASH_ADD_PTR(registered_handles, pointer, entry);
	_starpu_spin_unlock(&registered_handles_lock);
}

starpu_data_handle_t starpu_data_lookup(const void *ptr)
{
	starpu_data_handle_t result;

	_starpu_spin_lock(&registered_handles_lock);
	{
		struct handle_entry *entry;

		HASH_FIND_PTR(registered_handles, &ptr, entry);
		if(STARPU_UNLIKELY(entry == NULL))
			result = NULL;
		else
			result = entry->handle;
	}
	_starpu_spin_unlock(&registered_handles_lock);

	return result;
}

int
_starpu_data_is_multiformat_handle(starpu_data_handle_t handle)
{
	return handle->ops->is_multiformat;
}
/*
 * Start monitoring a piece of data
 */

static void _starpu_register_new_data(starpu_data_handle_t handle,
					unsigned home_node, uint32_t wt_mask)
{
	void *ptr;

	STARPU_ASSERT(handle);

	/* initialize the new lock */
	handle->req_list = _starpu_data_requester_list_new();
	handle->refcnt = 0;
	handle->busy_count = 0;
	handle->busy_waiting = 0;
	STARPU_PTHREAD_MUTEX_INIT(&handle->busy_mutex, NULL);
	STARPU_PTHREAD_COND_INIT(&handle->busy_cond, NULL);
	_starpu_spin_init(&handle->header_lock);

	/* first take care to properly lock the data */
	_starpu_spin_lock(&handle->header_lock);

	/* there is no hierarchy yet */
	handle->nchildren = 0;
	handle->root_handle = handle;
	handle->father_handle = NULL;
	handle->sibling_index = 0; /* could be anything for the root */
	handle->depth = 1; /* the tree is just a node yet */
        handle->rank = -1; /* invalid until set */
	handle->tag = -1; /* invalid until set */

	handle->is_not_important = 0;

	handle->sequential_consistency =
		starpu_data_get_default_sequential_consistency_flag();

	STARPU_PTHREAD_MUTEX_INIT(&handle->sequential_consistency_mutex, NULL);
	handle->last_submitted_mode = STARPU_R;
	handle->last_sync_task = NULL;
	handle->last_submitted_accessors = NULL;
	handle->post_sync_tasks = NULL;

	/* Tell helgrind that the race in _starpu_unlock_post_sync_tasks is fine */
	STARPU_HG_DISABLE_CHECKING(handle->post_sync_tasks_cnt);
	handle->post_sync_tasks_cnt = 0;

	/* By default, there are no methods available to perform a reduction */
	handle->redux_cl = NULL;
	handle->init_cl = NULL;

	handle->reduction_refcnt = 0;
	handle->reduction_req_list = _starpu_data_requester_list_new();

#ifdef STARPU_USE_FXT
	handle->last_submitted_ghost_sync_id_is_valid = 0;
	handle->last_submitted_ghost_sync_id = 0;
	handle->last_submitted_ghost_accessors_id = NULL;
#endif

	handle->wt_mask = wt_mask;

	/* Store some values directly in the handle not to recompute them all
	 * the time. */
	handle->footprint = _starpu_compute_data_footprint(handle);

	handle->home_node = home_node;

	/* that new data is invalid from all nodes perpective except for the
	 * home node */
	unsigned node;
	for (node = 0; node < STARPU_MAXNODES; node++)
	{
		struct _starpu_data_replicate *replicate;
		replicate = &handle->per_node[node];

		replicate->memory_node = node;
		replicate->relaxed_coherency = 0;
		replicate->refcnt = 0;

		if (node == home_node)
		{
			/* this is the home node with the only valid copy */
			replicate->state = STARPU_OWNER;
			replicate->allocated = 1;
			replicate->automatically_allocated = 0;
		}
		else
		{
			/* the value is not available here yet */
			replicate->state = STARPU_INVALID;
			replicate->allocated = 0;
		}
	}

	unsigned worker;
	unsigned nworkers = starpu_worker_get_count();
	for (worker = 0; worker < nworkers; worker++)
	{
		struct _starpu_data_replicate *replicate;
		replicate = &handle->per_worker[worker];
		replicate->allocated = 0;
		replicate->automatically_allocated = 0;
		replicate->state = STARPU_INVALID;
		replicate->refcnt = 0;
		replicate->handle = handle;

		for (node = 0; node < STARPU_MAXNODES; node++)
		{
			replicate->requested[node] = 0;
			replicate->request[node] = NULL;
		}

		/* Assuming being used for SCRATCH for now, patched when entering REDUX mode */
		replicate->relaxed_coherency = 1;
		replicate->initialized = 0;
		replicate->memory_node = starpu_worker_get_memory_node(worker);

		/* duplicate  the content of the interface on node 0 */
		memcpy(replicate->data_interface, handle->per_node[0].data_interface, handle->ops->interface_size);
	}

	/* now the data is available ! */
	_starpu_spin_unlock(&handle->header_lock);

	ptr = starpu_data_handle_to_pointer(handle, STARPU_MAIN_RAM);
	if (ptr != NULL)
	{
		_starpu_data_register_ram_pointer(handle, ptr);
	}
}

int _starpu_data_handle_init(starpu_data_handle_t handle, struct starpu_data_interface_ops *interface_ops, unsigned int mf_node)
{
	unsigned node;
	unsigned worker;

	/* Tell helgrind that our access to busy_count in
	 * starpu_data_unregister is actually safe */
	STARPU_HG_DISABLE_CHECKING(handle->busy_count);

	handle->ops = interface_ops;
	handle->mf_node = mf_node;

	size_t interfacesize = interface_ops->interface_size;

	_starpu_memory_stats_init(handle);
	for (node = 0; node < STARPU_MAXNODES; node++)
	{
		_starpu_memory_stats_init_per_node(handle, node);

		struct _starpu_data_replicate *replicate;
		replicate = &handle->per_node[node];
		/* relaxed_coherency = 0 */

		replicate->handle = handle;

		replicate->data_interface = calloc(1, interfacesize);
		STARPU_ASSERT(replicate->data_interface);
	}

	unsigned nworkers = starpu_worker_get_count();
	for (worker = 0; worker < nworkers; worker++)
	{
		struct _starpu_data_replicate *replicate;
		replicate = &handle->per_worker[worker];

		replicate->handle = handle;

		replicate->data_interface = calloc(1, interfacesize);
		STARPU_ASSERT(replicate->data_interface);

	}

	handle->tag = -1;
	handle->rank = -1;

	return 0;
}

static
starpu_data_handle_t _starpu_data_handle_allocate(struct starpu_data_interface_ops *interface_ops, unsigned int mf_node)
{
	starpu_data_handle_t handle = (starpu_data_handle_t) calloc(1, sizeof(struct _starpu_data_state));
	STARPU_ASSERT(handle);
	_starpu_data_handle_init(handle, interface_ops, mf_node);
	return handle;
}

void starpu_data_register(starpu_data_handle_t *handleptr, unsigned home_node,
			  void *data_interface,
			  struct starpu_data_interface_ops *ops)
{
	starpu_data_handle_t handle = _starpu_data_handle_allocate(ops, home_node);

	STARPU_ASSERT(handleptr);
	*handleptr = handle;

	/* fill the interface fields with the appropriate method */
	STARPU_ASSERT(ops->register_data_handle);
	ops->register_data_handle(handle, home_node, data_interface);

	_starpu_register_new_data(handle, home_node, 0);
}

void starpu_data_register_same(starpu_data_handle_t *handledst, starpu_data_handle_t handlesrc)
{
	void *local_interface = starpu_data_get_interface_on_node(handlesrc, STARPU_MAIN_RAM);
	starpu_data_register(handledst, -1, local_interface, handlesrc->ops);
}

void *starpu_data_handle_to_pointer(starpu_data_handle_t handle, unsigned node)
{
	/* Check whether the operation is supported and the node has actually
	 * been allocated.  */
	if (handle->ops->handle_to_pointer
	    && starpu_data_test_if_allocated_on_node(handle, node))
	{
		return handle->ops->handle_to_pointer(handle, node);
	}

	return NULL;
}

void *starpu_data_get_local_ptr(starpu_data_handle_t handle)
{
	return starpu_data_handle_to_pointer(handle,
					_starpu_memory_node_get_local_key());
}

int starpu_data_get_rank(starpu_data_handle_t handle)
{
	return handle->rank;
}

int _starpu_data_set_rank(starpu_data_handle_t handle, int rank)
{
	handle->rank = rank;
	return 0;
}

int starpu_data_set_rank(starpu_data_handle_t handle, int rank)
{
	_STARPU_DISP("Warning: You should call starpu_mpi_data_register which will insure MPI cache will be cleared when unregistering the data\n");
	return _starpu_data_set_rank(handle, rank);
}

int starpu_data_get_tag(starpu_data_handle_t handle)
{
	return handle->tag;
}

starpu_data_handle_t _starpu_data_get_data_handle_from_tag(int tag)
{
	struct handle_tag_entry *ret;

	_starpu_spin_lock(&registered_tag_handles_lock);
	HASH_FIND_INT(registered_tag_handles, &tag, ret);
	_starpu_spin_unlock(&registered_tag_handles_lock);

	if (ret)
	{
		return ret->handle;
	}
	else
	{
		return NULL;
	}
}

int _starpu_data_set_tag(starpu_data_handle_t handle, int tag)
{
	struct handle_tag_entry *entry;
	entry = (struct handle_tag_entry *) malloc(sizeof(*entry));
	STARPU_ASSERT(entry != NULL);

	STARPU_ASSERT_MSG(!(_starpu_data_get_data_handle_from_tag(tag)),
			  "There is already a data handle %p registered with the tag %d\n", _starpu_data_get_data_handle_from_tag(tag), tag);

	entry->tag = tag;
	entry->handle = handle;

	_starpu_spin_lock(&registered_tag_handles_lock);
	HASH_ADD_INT(registered_tag_handles, tag, entry);
	_starpu_spin_unlock(&registered_tag_handles_lock);

	handle->tag = tag;
	return 0;
}

int starpu_data_set_tag(starpu_data_handle_t handle, int tag)
{
	_STARPU_DISP("Warning: You should call starpu_mpi_data_register which will insure MPI cache will be cleared when unregistering the data\n");
	return _starpu_data_set_tag(handle, tag);
}

static
int _starpu_data_release_tag(starpu_data_handle_t handle)
{
	struct handle_tag_entry *tag_entry;

	if (handle->tag != -1)
	{
		_starpu_spin_lock(&registered_tag_handles_lock);
		HASH_FIND_INT(registered_tag_handles, &handle->tag, tag_entry);
		STARPU_ASSERT_MSG((tag_entry != NULL),"Data handle %p with tag %d isn't in the hashmap !",handle,handle->tag);

		HASH_DEL(registered_tag_handles, tag_entry);
		free(tag_entry);

		_starpu_spin_unlock(&registered_tag_handles_lock);
	}
	return 0;
}

struct starpu_data_interface_ops* starpu_data_get_interface_ops(starpu_data_handle_t handle)
{
	return handle->ops;
}

/*
 * Stop monitoring a piece of data
 */

void _starpu_data_unregister_ram_pointer(starpu_data_handle_t handle)
{
	const void *ram_ptr = starpu_data_handle_to_pointer(handle, STARPU_MAIN_RAM);
	if (ram_ptr != NULL)
	{
		/* Remove the PTR -> HANDLE mapping.  If a mapping from PTR
		 * to another handle existed before (e.g., when using
		 * filters), it becomes visible again.  */
		struct handle_entry *entry;

		_starpu_spin_lock(&registered_handles_lock);
		HASH_FIND_PTR(registered_handles, &ram_ptr, entry);
		STARPU_ASSERT(entry != NULL);

		HASH_DEL(registered_handles, entry);
		free(entry);

		_starpu_spin_unlock(&registered_handles_lock);
	}
}

void _starpu_data_free_interfaces(starpu_data_handle_t handle)
{
	unsigned node;
	unsigned worker;
	unsigned nworkers = starpu_worker_get_count();

	for (node = 0; node < STARPU_MAXNODES; node++)
		free(handle->per_node[node].data_interface);

	for (worker = 0; worker < nworkers; worker++)
		free(handle->per_worker[worker].data_interface);
}

struct _starpu_unregister_callback_arg
{
	unsigned memory_node;
	starpu_data_handle_t handle;
	unsigned terminated;
	starpu_pthread_mutex_t mutex;
	starpu_pthread_cond_t cond;
};

/* Check whether we should tell starpu_data_unregister that the data handle is
 * not busy any more.
 * The header is supposed to be locked.
 * This may free the handle, if it was lazily unregistered (1 is returned in
 * that case).  The handle pointer thus becomes invalid for the caller.
 */
int _starpu_data_check_not_busy(starpu_data_handle_t handle)
{
	if (!handle->busy_count && handle->busy_waiting)
	{
		STARPU_PTHREAD_MUTEX_LOCK(&handle->busy_mutex);
		STARPU_PTHREAD_COND_BROADCAST(&handle->busy_cond);
		STARPU_PTHREAD_MUTEX_UNLOCK(&handle->busy_mutex);
	}

	/* The handle has been destroyed in between (eg. this was a temporary
	 * handle created for a reduction.) */
	if (handle->lazy_unregister && handle->busy_count == 0)
	{
		_starpu_spin_unlock(&handle->header_lock);
		starpu_data_unregister_no_coherency(handle);
		/* Warning: in case we unregister the handle, we must be sure
		 * that the caller will not try to unlock the header after
		 * !*/
		return 1;
	}

	return 0;
}

static void _starpu_data_unregister_fetch_data_callback(void *_arg)
{
	int ret;
	struct _starpu_unregister_callback_arg *arg = (struct _starpu_unregister_callback_arg *) _arg;

	starpu_data_handle_t handle = arg->handle;

	STARPU_ASSERT(handle);

	struct _starpu_data_replicate *replicate = &handle->per_node[arg->memory_node];

	ret = _starpu_fetch_data_on_node(handle, replicate, STARPU_R, 0, 0, NULL, NULL);
	STARPU_ASSERT(!ret);

	/* unlock the caller */
	STARPU_PTHREAD_MUTEX_LOCK(&arg->mutex);
	arg->terminated = 1;
	STARPU_PTHREAD_COND_SIGNAL(&arg->cond);
	STARPU_PTHREAD_MUTEX_UNLOCK(&arg->mutex);
}

/* Unregister the data handle, perhaps we don't need to update the home_node
 * (in that case coherent is set to 0) */
static void _starpu_data_unregister(starpu_data_handle_t handle, unsigned coherent)
{
	STARPU_ASSERT(handle);
	STARPU_ASSERT_MSG(handle->nchildren == 0, "data %p needs to be unpartitioned before unregistration", handle);

	if (coherent)
	{
		STARPU_ASSERT_MSG(_starpu_worker_may_perform_blocking_calls(), "starpu_data_unregister must not be called from a task or callback, perhaps you can use starpu_data_unregister_submit instead");

		/* If sequential consistency is enabled, wait until data is available */
		_starpu_data_wait_until_available(handle, STARPU_RW);

		/* Fetch data in the home of the data to ensure we have a valid copy
		 * where we registered it */
		int home_node = handle->home_node;
		if (home_node >= 0)
		{
			struct _starpu_unregister_callback_arg arg;
			arg.handle = handle;
			arg.memory_node = (unsigned)home_node;
			arg.terminated = 0;
			STARPU_PTHREAD_MUTEX_INIT(&arg.mutex, NULL);
			STARPU_PTHREAD_COND_INIT(&arg.cond, NULL);

			if (!_starpu_attempt_to_submit_data_request_from_apps(handle, STARPU_R,
					_starpu_data_unregister_fetch_data_callback, &arg))
			{
				/* no one has locked this data yet, so we proceed immediately */
				struct _starpu_data_replicate *home_replicate = &handle->per_node[home_node];
				int ret = _starpu_fetch_data_on_node(handle, home_replicate, STARPU_R, 0, 0, NULL, NULL);
				STARPU_ASSERT(!ret);
			}
			else
			{
				STARPU_PTHREAD_MUTEX_LOCK(&arg.mutex);
				while (!arg.terminated)
					STARPU_PTHREAD_COND_WAIT(&arg.cond, &arg.mutex);
				STARPU_PTHREAD_MUTEX_UNLOCK(&arg.mutex);
			}
			STARPU_PTHREAD_MUTEX_DESTROY(&arg.mutex);
			STARPU_PTHREAD_COND_DESTROY(&arg.cond);
			_starpu_release_data_on_node(handle, 0, &handle->per_node[home_node]);
		}

		/* If this handle uses a multiformat interface, we may have to convert
		 * this piece of data back into the CPU format.
		 * XXX : This is quite hacky, could we submit a task instead ?
		 */
		if (_starpu_data_is_multiformat_handle(handle) &&
			(  starpu_node_get_kind(handle->mf_node) != STARPU_CPU_RAM
			&& starpu_node_get_kind(handle->mf_node) != STARPU_SCC_RAM
			&& starpu_node_get_kind(handle->mf_node) != STARPU_SCC_SHM
			 ))
		{
			_STARPU_DEBUG("Conversion needed\n");
			void *buffers[1];
			struct starpu_multiformat_interface *format_interface;
			format_interface = (struct starpu_multiformat_interface *) starpu_data_get_interface_on_node(handle, STARPU_MAIN_RAM);
			struct starpu_codelet *cl = NULL;
			enum starpu_node_kind node_kind = starpu_node_get_kind(handle->mf_node);

			switch (node_kind)
			{
#ifdef STARPU_USE_CUDA
				case STARPU_CUDA_RAM:
				{
					struct starpu_multiformat_data_interface_ops *mf_ops;
					mf_ops = (struct starpu_multiformat_data_interface_ops *) handle->ops->get_mf_ops(format_interface);
					cl = mf_ops->cuda_to_cpu_cl;
					break;
				}
#endif
#ifdef STARPU_USE_OPENCL
				case STARPU_OPENCL_RAM:
				{
					struct starpu_multiformat_data_interface_ops *mf_ops;
					mf_ops = (struct starpu_multiformat_data_interface_ops *) handle->ops->get_mf_ops(format_interface);
					cl = mf_ops->opencl_to_cpu_cl;
					break;
				}
#endif
#ifdef STARPU_USE_MIC
				case STARPU_MIC_RAM:
				{
					struct starpu_multiformat_data_interface_ops *mf_ops;
					mf_ops = (struct starpu_multiformat_data_interface_ops *) handle->ops->get_mf_ops(format_interface);
					cl = mf_ops->mic_to_cpu_cl;
					break;
				}
#endif
				case STARPU_CPU_RAM:      /* Impossible ! */
				case STARPU_SCC_RAM:      /* Impossible ! */
				case STARPU_SCC_SHM:      /* Impossible ! */
				default:
					STARPU_ABORT();
			}
			buffers[0] = format_interface;

			_starpu_cl_func_t func = _starpu_task_get_cpu_nth_implementation(cl, 0);
			STARPU_ASSERT(func);
			func(buffers, NULL);
		}
	}

	_starpu_spin_lock(&handle->header_lock);
	if (!coherent)
	{
		/* Should we postpone the unregister operation ? */
		if ((handle->busy_count > 0) && handle->lazy_unregister)
		{
			_starpu_spin_unlock(&handle->header_lock);
			return;
		}
	}

	/* Tell holders of references that we're starting waiting */
	handle->busy_waiting = 1;
	_starpu_spin_unlock(&handle->header_lock);

	/* Wait for all requests to finish (notably WT requests) */
	STARPU_PTHREAD_MUTEX_LOCK(&handle->busy_mutex);
	while (1) {
		/* Here helgrind would shout that this an unprotected access,
		 * but this is actually fine: all threads who do busy_count--
		 * are supposed to call _starpu_data_check_not_busy, which will
		 * wake us up through the busy_mutex/busy_cond. */
		if (!handle->busy_count)
			break;
		/* This is woken by _starpu_data_check_not_busy, always called
		 * after decrementing busy_count */
		STARPU_PTHREAD_COND_WAIT(&handle->busy_cond, &handle->busy_mutex);
	}
	STARPU_PTHREAD_MUTEX_UNLOCK(&handle->busy_mutex);

	/* Wait for finished requests to release the handle */
	_starpu_spin_lock(&handle->header_lock);

	size_t size = _starpu_data_get_size(handle);

	if (handle->unregister_hook)
	{
		_starpu_spin_unlock(&handle->header_lock);
		handle->unregister_hook(handle);
		_starpu_spin_lock(&handle->header_lock);
	}

	_starpu_data_unregister_ram_pointer(handle);
	_starpu_data_free_interfaces(handle);

	/* Destroy the data now */
	unsigned node;
	for (node = 0; node < STARPU_MAXNODES; node++)
	{
		struct _starpu_data_replicate *local = &handle->per_node[node];
		/* free the data copy in a lazy fashion */
		if (local->allocated && local->automatically_allocated)
			_starpu_request_mem_chunk_removal(handle, local, node, size);
	}
	unsigned worker;
	unsigned nworkers = starpu_worker_get_count();
	for (worker = 0; worker < nworkers; worker++)
	{
		struct _starpu_data_replicate *local = &handle->per_worker[worker];
		/* free the data copy in a lazy fashion */
		if (local->allocated && local->automatically_allocated)
			_starpu_request_mem_chunk_removal(handle, local, starpu_worker_get_memory_node(worker), size);
	}

	_starpu_memory_stats_free(handle);
	_starpu_data_requester_list_delete(handle->req_list);
	_starpu_data_requester_list_delete(handle->reduction_req_list);

	_starpu_spin_unlock(&handle->header_lock);
	_starpu_spin_destroy(&handle->header_lock);

	STARPU_PTHREAD_MUTEX_DESTROY(&handle->busy_mutex);
	STARPU_PTHREAD_COND_DESTROY(&handle->busy_cond);
	STARPU_PTHREAD_MUTEX_DESTROY(&handle->sequential_consistency_mutex);

	_starpu_data_release_tag(handle);

	free(handle);
}

void starpu_data_unregister(starpu_data_handle_t handle)
{
	STARPU_ASSERT_MSG(!handle->lazy_unregister, "data %p can not be unregistered twice", handle);
	_starpu_data_unregister(handle, 1);
}

void starpu_data_unregister_no_coherency(starpu_data_handle_t handle)
{
	_starpu_data_unregister(handle, 0);
}

void starpu_data_unregister_submit(starpu_data_handle_t handle)
{
	_starpu_spin_lock(&handle->header_lock);
	STARPU_ASSERT_MSG(!handle->lazy_unregister, "data %p can not be unregistered twice", handle);
	handle->lazy_unregister = 1;
	_starpu_spin_unlock(&handle->header_lock);
	_starpu_data_unregister(handle, 0);
}

static void _starpu_data_invalidate(void *data)
{
	starpu_data_handle_t handle = data;
	size_t size = _starpu_data_get_size(handle);
	_starpu_spin_lock(&handle->header_lock);

	unsigned node;
	for (node = 0; node < STARPU_MAXNODES; node++)
	{
		struct _starpu_data_replicate *local = &handle->per_node[node];

		if (local->mc && local->allocated && local->automatically_allocated)
		{
			if (node == STARPU_MAIN_RAM)
				_starpu_data_unregister_ram_pointer(handle);

			/* free the data copy in a lazy fashion */
			_starpu_request_mem_chunk_removal(handle, local, node, size);
		}

		local->state = STARPU_INVALID;
	}

	unsigned worker;
	unsigned nworkers = starpu_worker_get_count();
	for (worker = 0; worker < nworkers; worker++)
	{
		struct _starpu_data_replicate *local = &handle->per_worker[worker];

		if (local->mc && local->allocated && local->automatically_allocated)
			/* free the data copy in a lazy fashion */
			_starpu_request_mem_chunk_removal(handle, local, starpu_worker_get_memory_node(worker), size);

		local->state = STARPU_INVALID;
	}

	_starpu_spin_unlock(&handle->header_lock);

	starpu_data_release_on_node(handle, -1);
}

void starpu_data_invalidate(starpu_data_handle_t handle)
{
	STARPU_ASSERT(handle);

	starpu_data_acquire_on_node(handle, -1, STARPU_W);

	_starpu_data_invalidate(handle);
}

void starpu_data_invalidate_submit(starpu_data_handle_t handle)
{
	STARPU_ASSERT(handle);

	starpu_data_acquire_on_node_cb(handle, -1, STARPU_W, _starpu_data_invalidate, handle);
}

enum starpu_data_interface_id starpu_data_get_interface_id(starpu_data_handle_t handle)
{
	return handle->ops->interfaceid;
}

void *starpu_data_get_interface_on_node(starpu_data_handle_t handle, unsigned memory_node)
{
	return handle->per_node[memory_node].data_interface;
}

int starpu_data_interface_get_next_id(void)
{
	_data_interface_number += 1;
	return _data_interface_number-1;
}

int starpu_data_pack(starpu_data_handle_t handle, void **ptr, starpu_ssize_t *count)
{
	STARPU_ASSERT(handle->ops->pack_data);
	return handle->ops->pack_data(handle, _starpu_memory_node_get_local_key(), ptr, count);
}

int starpu_data_unpack(starpu_data_handle_t handle, void *ptr, size_t count)
{
	STARPU_ASSERT(handle->ops->unpack_data);
	int ret;
	ret = handle->ops->unpack_data(handle, _starpu_memory_node_get_local_key(), ptr, count);
	free(ptr);
	return ret;
}

size_t starpu_data_get_size(starpu_data_handle_t handle)
{
	return handle->ops->get_size(handle);
}
