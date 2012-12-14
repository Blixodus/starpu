/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2010-2012  Université de Bordeaux 1
 * Copyright (C) 2010-2012  Centre National de la Recherche Scientifique
 * Copyright (C) 2011  INRIA
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

#include <pthread.h>

#include <starpu.h>
#include <common/config.h>
#include <common/utils.h>
#include <core/sched_policy.h>
#include <profiling/profiling.h>
#include <common/barrier.h>
#include <core/debug.h>

static int use_prefetch = 0;

int starpu_get_prefetch_flag(void)
{
	return use_prefetch;
}

static struct starpu_sched_policy *predefined_policies[] =
{
	&_starpu_sched_eager_policy,
	&_starpu_sched_prio_policy,
	&_starpu_sched_random_policy,
	&_starpu_sched_ws_policy,
	&_starpu_sched_dm_policy,
	&_starpu_sched_dmda_policy,
	&_starpu_sched_dmda_ready_policy,
	&_starpu_sched_dmda_sorted_policy,
	&_starpu_sched_parallel_heft_policy,
	&_starpu_sched_peager_policy,
	NULL
};

struct starpu_sched_policy **starpu_sched_get_predefined_policies()
{
	return predefined_policies;
}

struct starpu_sched_policy *_starpu_get_sched_policy(struct _starpu_sched_ctx *sched_ctx)
{
	return sched_ctx->sched_policy;
}

/*
 *	Methods to initialize the scheduling policy
 */

static void load_sched_policy(struct starpu_sched_policy *sched_policy, struct _starpu_sched_ctx *sched_ctx)
{
	STARPU_ASSERT(sched_policy);

#ifdef STARPU_VERBOSE
	if (sched_policy->policy_name)
	{
		if (sched_policy->policy_description)
                        _STARPU_DEBUG("Use %s scheduler (%s)\n", sched_policy->policy_name, sched_policy->policy_description);
                else
                        _STARPU_DEBUG("Use %s scheduler \n", sched_policy->policy_name);

	}
#endif

	struct starpu_sched_policy *policy = sched_ctx->sched_policy;
	memcpy(policy, sched_policy, sizeof(*policy));
}

static struct starpu_sched_policy *find_sched_policy_from_name(const char *policy_name)
{
	if (!policy_name)
		return NULL;

	if (strncmp(policy_name, "heft", 5) == 0)
	{
		_STARPU_DISP("Warning: heft is now called \"dmda\".\n");
		return &_starpu_sched_dmda_policy;
	}

	unsigned i;
	for (i = 0; i < sizeof(predefined_policies)/sizeof(predefined_policies[0]); i++)
	{
		struct starpu_sched_policy *p;
		p = predefined_policies[i];
		if (p == NULL) continue; // We reached the end of the array
		if (p->policy_name)
		{
			if (strcmp(policy_name, p->policy_name) == 0)
			{
				/* we found a policy with the requested name */
				return p;
			}
		}
	}
	fprintf(stderr, "Warning: scheduling policy \"%s\" was not found, try \"help\" to get a list\n", policy_name);

	/* nothing was found */
	return NULL;
}

static void display_sched_help_message(void)
{
	const char *sched_env = getenv("STARPU_SCHED");
	if (sched_env && (strcmp(sched_env, "help") == 0))
	{
		fprintf(stderr, "STARPU_SCHED can be either of\n");

		/* display the description of all predefined policies */
		unsigned i;
		for (i = 0; i < sizeof(predefined_policies)/sizeof(predefined_policies[0]); i++)
		{
			struct starpu_sched_policy *p;
			p = predefined_policies[i];
			fprintf(stderr, "%s\t-> %s\n", p->policy_name, p->policy_description);
		}
	 }
}

static struct starpu_sched_policy *select_sched_policy(struct _starpu_machine_config *config, const char *required_policy)
{
	struct starpu_sched_policy *selected_policy = NULL;
	struct starpu_conf *user_conf = config->conf;

	if(required_policy)
		selected_policy = find_sched_policy_from_name(required_policy);

	/* First, we check whether the application explicitely gave a scheduling policy or not */
	if (!selected_policy && user_conf && (user_conf->sched_policy))
		return user_conf->sched_policy;

	/* Otherwise, we look if the application specified the name of a policy to load */
	const char *sched_pol_name;
	sched_pol_name = getenv("STARPU_SCHED");
	if (sched_pol_name == NULL && user_conf && user_conf->sched_policy_name)
		sched_pol_name = user_conf->sched_policy_name;

	if (!selected_policy && sched_pol_name)
		selected_policy = find_sched_policy_from_name(sched_pol_name);

	/* Perhaps there was no policy that matched the name */
	if (selected_policy)
		return selected_policy;

	/* If no policy was specified, we use the greedy policy as a default */
	return &_starpu_sched_eager_policy;
}

void _starpu_init_sched_policy(struct _starpu_machine_config *config, struct _starpu_sched_ctx *sched_ctx, const char *required_policy)
{
	/* Perhaps we have to display some help */
	display_sched_help_message();

	/* Prefetch is activated by default */
	use_prefetch = starpu_get_env_number("STARPU_PREFETCH");
	if (use_prefetch == -1)
		use_prefetch = 1;

	/* Set calibrate flag */
	_starpu_set_calibrate_flag(config->conf->calibrate);

	struct starpu_sched_policy *selected_policy;
	selected_policy = select_sched_policy(config, required_policy);

	load_sched_policy(selected_policy, sched_ctx);

	sched_ctx->sched_policy->init_sched(sched_ctx->id);
}

void _starpu_deinit_sched_policy(struct _starpu_sched_ctx *sched_ctx)
{
	struct starpu_sched_policy *policy = sched_ctx->sched_policy;
	if (policy->deinit_sched)
		policy->deinit_sched(sched_ctx->id);
}

/* Enqueue a task into the list of tasks explicitely attached to a worker. In
 * case workerid identifies a combined worker, a task will be enqueued into
 * each worker of the combination. */
static int _starpu_push_task_on_specific_worker(struct starpu_task *task, int workerid)
{
	int nbasic_workers = (int)starpu_worker_get_count();

	/* Is this a basic worker or a combined worker ? */
	int is_basic_worker = (workerid < nbasic_workers);

	unsigned memory_node;
	struct _starpu_worker *worker = NULL;
	struct _starpu_combined_worker *combined_worker = NULL;

	if (is_basic_worker)
	{
		worker = _starpu_get_worker_struct(workerid);
		memory_node = worker->memory_node;
	}
	else
	{
		combined_worker = _starpu_get_combined_worker_struct(workerid);
		memory_node = combined_worker->memory_node;
	}

	if (use_prefetch)
		starpu_prefetch_task_input_on_node(task, memory_node);

	/* if we push a task on a specific worker, notify all the sched_ctxs the worker belongs to */
	unsigned i;
	struct _starpu_sched_ctx *sched_ctx;
	for(i = 0; i < STARPU_NMAX_SCHED_CTXS; i++)
	{
		sched_ctx = worker->sched_ctx[i];
		if (sched_ctx != NULL && sched_ctx->sched_policy != NULL && sched_ctx->sched_policy->push_task_notify)
			sched_ctx->sched_policy->push_task_notify(task, workerid, sched_ctx->id);
	}

#ifdef STARPU_USE_SCHED_CTX_HYPERVISOR
	starpu_call_pushed_task_cb(workerid, task->sched_ctx);
#endif //STARPU_USE_SCHED_CTX_HYPERVISOR

	if (is_basic_worker)
	{
		unsigned node = starpu_worker_get_memory_node(workerid);
		if (_starpu_task_uses_multiformat_handles(task))
		{
			unsigned i;
			for (i = 0; i < task->cl->nbuffers; i++)
			{
				struct starpu_task *conversion_task;
				starpu_data_handle_t handle;

				handle = task->handles[i];
				if (!_starpu_handle_needs_conversion_task(handle, node))
					continue;

				conversion_task = _starpu_create_conversion_task(handle, node);
				conversion_task->mf_skip = 1;
				conversion_task->execute_on_a_specific_worker = 1;
				conversion_task->workerid = workerid;
				_starpu_task_submit_conversion_task(conversion_task, workerid);
				//_STARPU_DEBUG("Pushing a conversion task\n");
			}

			for (i = 0; i < task->cl->nbuffers; i++)
				task->handles[i]->mf_node = node;
		}
//		if(task->sched_ctx != _starpu_get_initial_sched_ctx()->id)
			
		if(task->priority > 0)
			return _starpu_push_local_task(worker, task, 1);
		else
			return _starpu_push_local_task(worker, task, 0);
	}
	else
	{
		/* This is a combined worker so we create task aliases */
		int worker_size = combined_worker->worker_size;
		int *combined_workerid = combined_worker->combined_workerid;

		int ret = 0;

		struct _starpu_job *j = _starpu_get_job_associated_to_task(task);
		j->task_size = worker_size;
		j->combined_workerid = workerid;
		j->active_task_alias_count = 0;

		_STARPU_PTHREAD_BARRIER_INIT(&j->before_work_barrier, NULL, worker_size);
		_STARPU_PTHREAD_BARRIER_INIT(&j->after_work_barrier, NULL, worker_size);

		int i;
		for (i = 0; i < worker_size; i++)
		{
			struct starpu_task *alias = _starpu_create_task_alias(task);

			worker = _starpu_get_worker_struct(combined_workerid[i]);
			ret |= _starpu_push_local_task(worker, alias, 0);
		}

		return ret;
	}
}

static int _starpu_nworkers_able_to_execute_task(struct starpu_task *task, struct _starpu_sched_ctx *sched_ctx)
{
	int worker = -1, nworkers = 0;
	struct starpu_sched_ctx_worker_collection *workers = sched_ctx->workers;
	if(workers->init_cursor)
		workers->init_cursor(workers);

	while(workers->has_next(workers))
	{
		worker = workers->get_next(workers);
		if (starpu_worker_can_execute_task(worker, task, 0) && starpu_is_ctxs_turn(worker, sched_ctx->id))
			nworkers++;
	}

	if (workers->deinit_cursor)
		workers->deinit_cursor(workers);
	return nworkers;
}

/* the generic interface that call the proper underlying implementation */

int _starpu_push_task(struct _starpu_job *j)
{
	struct starpu_task *task = j->task;
	struct _starpu_sched_ctx *sched_ctx = _starpu_get_sched_ctx_struct(task->sched_ctx);
	unsigned nworkers = 0;

	_STARPU_LOG_IN();

	if(!sched_ctx->is_initial_sched)
	{
		/*if there are workers in the ctx that are not able to execute tasks
		  we consider the ctx empty */
		nworkers = _starpu_nworkers_able_to_execute_task(task, sched_ctx);

		if(nworkers == 0)
		{
			if(task->already_pushed)
			{
				_STARPU_PTHREAD_MUTEX_LOCK(&sched_ctx->empty_ctx_mutex);
				starpu_task_list_push_back(&sched_ctx->empty_ctx_tasks, task);
				_STARPU_PTHREAD_MUTEX_UNLOCK(&sched_ctx->empty_ctx_mutex);
				_STARPU_LOG_OUT();
				return -1;
			}
			else
			{
				_STARPU_PTHREAD_MUTEX_LOCK(&sched_ctx->empty_ctx_mutex);
				task->already_pushed = 1;
				starpu_task_list_push_front(&sched_ctx->empty_ctx_tasks, task);
				_STARPU_PTHREAD_MUTEX_UNLOCK(&sched_ctx->empty_ctx_mutex);
				_STARPU_LOG_OUT();
				return 0;
			}
		}
	}

	_STARPU_TRACE_JOB_PUSH(task, task->priority > 0);
	_starpu_increment_nready_tasks();
	task->status = STARPU_TASK_READY;
#ifdef HAVE_AYUDAME_H
	if (AYU_event) {
		int id = -1;
		AYU_event(AYU_ADDTASKTOQUEUE, j->job_id, &id);
	}
#endif
	_starpu_profiling_set_task_push_start_time(task);

	/* in case there is no codelet associated to the task (that's a control
	 * task), we directly execute its callback and enforce the
	 * corresponding dependencies */
	if (task->cl == NULL)
	{
		_starpu_handle_job_termination(j);
		_STARPU_LOG_OUT_TAG("handle_job_termination");
		return 0;
	}

	int ret;
	if (STARPU_UNLIKELY(task->execute_on_a_specific_worker))
	{
		ret = _starpu_push_task_on_specific_worker(task, task->workerid);
	}
	else
	{
		STARPU_ASSERT(sched_ctx->sched_policy->push_task);
		ret = sched_ctx->sched_policy->push_task(task);
		if(ret == -1)
		{
			fprintf(stderr, "repush task \n");
			_STARPU_TRACE_JOB_POP(task, task->priority > 0);
			_starpu_decrement_nready_tasks();
			ret = _starpu_push_task(j);
		}
	}

	_starpu_profiling_set_task_push_end_time(task);

	_STARPU_LOG_OUT();
	return ret;
}

/*
 * Given a handle that needs to be converted in order to be used on the given
 * node, returns a task that takes care of the conversion.
 */
struct starpu_task *_starpu_create_conversion_task(starpu_data_handle_t handle,
						   unsigned int node)
{
	struct starpu_task *conversion_task;
	struct starpu_multiformat_interface *format_interface;
	enum starpu_node_kind node_kind;

	conversion_task = starpu_task_create();
	conversion_task->synchronous = 0;
	conversion_task->handles[0] = handle;

	/* The node does not really matter here */
	format_interface = (struct starpu_multiformat_interface *) starpu_data_get_interface_on_node(handle, 0);
	node_kind = starpu_node_get_kind(node);

	_starpu_spin_lock(&handle->header_lock);
	handle->refcnt++;
	handle->busy_count++;
	_starpu_spin_unlock(&handle->header_lock);

	switch(node_kind)
	{
	case STARPU_CPU_RAM:
		switch (starpu_node_get_kind(handle->mf_node))
		{
		case STARPU_CPU_RAM:
			STARPU_ABORT();
#ifdef STARPU_USE_CUDA
		case STARPU_CUDA_RAM:
		{
			struct starpu_multiformat_data_interface_ops *mf_ops;
			mf_ops = (struct starpu_multiformat_data_interface_ops *) handle->ops->get_mf_ops(format_interface);
			conversion_task->cl = mf_ops->cuda_to_cpu_cl;
			break;
		}
#endif
#ifdef STARPU_USE_OPENCL
		case STARPU_OPENCL_RAM:
		{
			struct starpu_multiformat_data_interface_ops *mf_ops;
			mf_ops = (struct starpu_multiformat_data_interface_ops *) handle->ops->get_mf_ops(format_interface);
			conversion_task->cl = mf_ops->opencl_to_cpu_cl;
			break;
		}
#endif
		default:
			_STARPU_ERROR("Oops : %u\n", handle->mf_node);
		}
		break;
#ifdef STARPU_USE_CUDA
	case STARPU_CUDA_RAM:
		{
			struct starpu_multiformat_data_interface_ops *mf_ops;
			mf_ops = (struct starpu_multiformat_data_interface_ops *) handle->ops->get_mf_ops(format_interface);
			conversion_task->cl = mf_ops->cpu_to_cuda_cl;
			break;
		}
#endif
#ifdef STARPU_USE_OPENCL
	case STARPU_OPENCL_RAM:
	{
		struct starpu_multiformat_data_interface_ops *mf_ops;
		mf_ops = (struct starpu_multiformat_data_interface_ops *) handle->ops->get_mf_ops(format_interface);
		conversion_task->cl = mf_ops->cpu_to_opencl_cl;
		break;
	}
#endif
	case STARPU_SPU_LS: /* Not supported */
	default:
		STARPU_ABORT();
	}

	conversion_task->cl->modes[0] = STARPU_RW;
	return conversion_task;
}

struct _starpu_sched_ctx* _get_next_sched_ctx_to_pop_into(struct _starpu_worker *worker)
{
	struct _starpu_sched_ctx *sched_ctx, *good_sched_ctx = NULL;
	unsigned smallest_counter =  worker->nsched_ctxs;
	unsigned i;
	for(i = 0; i < STARPU_NMAX_SCHED_CTXS; i++)
	{
		sched_ctx = worker->sched_ctx[i];

		if(sched_ctx != NULL && sched_ctx->id != STARPU_NMAX_SCHED_CTXS &&
		   sched_ctx->pop_counter[worker->workerid] < worker->nsched_ctxs &&
		   smallest_counter > sched_ctx->pop_counter[worker->workerid])
		{
			good_sched_ctx = sched_ctx;
			smallest_counter = sched_ctx->pop_counter[worker->workerid];
		}
	}

	if(good_sched_ctx == NULL)
	{
		for(i = 0; i < STARPU_NMAX_SCHED_CTXS; i++)
		{
			sched_ctx = worker->sched_ctx[i];
			if(sched_ctx != NULL && sched_ctx->id != STARPU_NMAX_SCHED_CTXS)
				sched_ctx->pop_counter[worker->workerid] = 0;
		}

		return _get_next_sched_ctx_to_pop_into(worker);
	}
	return good_sched_ctx;
}

struct starpu_task *_starpu_pop_task(struct _starpu_worker *worker)
{
	struct starpu_task *task;
	int worker_id;
	unsigned node;

	/* We can't tell in advance which task will be picked up, so we measure
	 * a timestamp, and will attribute it afterwards to the task. */
	int profiling = starpu_profiling_status_get();
	struct timespec pop_start_time;
	if (profiling)
		_starpu_clock_gettime(&pop_start_time);

pick:
	/* perhaps there is some local task to be executed first */
	task = _starpu_pop_local_task(worker);


	/* get tasks from the stacks of the strategy */
	if(!task)
	{
		struct _starpu_sched_ctx *sched_ctx;

		int been_here[STARPU_NMAX_SCHED_CTXS];
		int i;
		for(i = 0; i < STARPU_NMAX_SCHED_CTXS; i++)
			been_here[i] = 0;

		while(!task)
		{
			if(worker->nsched_ctxs == 1)
				sched_ctx = _starpu_get_initial_sched_ctx();
			else
				sched_ctx = _get_next_sched_ctx_to_pop_into(worker);

			if(sched_ctx != NULL && sched_ctx->id != STARPU_NMAX_SCHED_CTXS)
			{

				if (sched_ctx->sched_policy && sched_ctx->sched_policy->pop_task)
					task = sched_ctx->sched_policy->pop_task(sched_ctx->id);
			}

			if((!task && sched_ctx->pop_counter[worker->workerid] == 0 && been_here[sched_ctx->id]) || worker->nsched_ctxs == 1)
				break;


			been_here[sched_ctx->id] = 1;

			sched_ctx->pop_counter[worker->workerid]++;

		}

	  }

#ifdef STARPU_USE_SCHED_CTX_HYPERVISOR
	struct _starpu_sched_ctx *sched_ctx = NULL;
	struct starpu_performance_counters *perf_counters = NULL;
	int j;
	for(j = 0; j < STARPU_NMAX_SCHED_CTXS; j++)
	{
		sched_ctx = worker->sched_ctx[j];
		if(sched_ctx != NULL && sched_ctx->id != 0)
		{
			perf_counters = sched_ctx->perf_counters;
			if(perf_counters != NULL && perf_counters->notify_idle_cycle && perf_counters->notify_idle_end)
			{
				if(!task)
					perf_counters->notify_idle_cycle(sched_ctx->id, worker->workerid, 1.0);
				else
					perf_counters->notify_idle_end(sched_ctx->id, worker->workerid);
			}
		}
	}
#endif //STARPU_USE_SCHED_CTX_HYPERVISOR


	if (!task)
		return NULL;

	/* Make sure we do not bother with all the multiformat-specific code if
	 * it is not necessary. */
	if (!_starpu_task_uses_multiformat_handles(task))
		goto profiling;


	/* This is either a conversion task, or a regular task for which the
	 * conversion tasks have already been created and submitted */
	if (task->mf_skip)
		goto profiling;

	worker_id = starpu_worker_get_id();
	if (!starpu_worker_can_execute_task(worker_id, task, 0))
		return task;

	node = starpu_worker_get_memory_node(worker_id);

	/*
	 * We do have a task that uses multiformat handles. Let's create the
	 * required conversion tasks.
	 */
	unsigned i;
	for (i = 0; i < task->cl->nbuffers; i++)
	{
		struct starpu_task *conversion_task;
		starpu_data_handle_t handle;

		handle = task->handles[i];
		if (!_starpu_handle_needs_conversion_task(handle, node))
			continue;
		conversion_task = _starpu_create_conversion_task(handle, node);
		conversion_task->mf_skip = 1;
		conversion_task->execute_on_a_specific_worker = 1;
		conversion_task->workerid = worker_id;
		/*
		 * Next tasks will need to know where these handles have gone.
		 */
		handle->mf_node = node;
		_starpu_task_submit_conversion_task(conversion_task, worker_id);
	}

	task->mf_skip = 1;
	starpu_task_list_push_front(&worker->local_tasks, task);
	goto pick;

profiling:
	if (profiling)
	{
		struct starpu_task_profiling_info *profiling_info;
		profiling_info = task->profiling_info;

		/* The task may have been created before profiling was enabled,
		 * so we check if the profiling_info structure is available
		 * even though we already tested if profiling is enabled. */
		if (profiling_info)
		{
			memcpy(&profiling_info->pop_start_time,
				&pop_start_time, sizeof(struct timespec));
			_starpu_clock_gettime(&profiling_info->pop_end_time);
		}
	}

	return task;
}

struct starpu_task *_starpu_pop_every_task(struct _starpu_sched_ctx *sched_ctx)
{
	STARPU_ASSERT(sched_ctx->sched_policy->pop_every_task);

	/* TODO set profiling info */
	if(sched_ctx->sched_policy->pop_every_task)
		return sched_ctx->sched_policy->pop_every_task(sched_ctx->id);
	return NULL;
}

void _starpu_sched_pre_exec_hook(struct starpu_task *task)
{
	struct _starpu_sched_ctx *sched_ctx = _starpu_get_sched_ctx_struct(task->sched_ctx);
	if (sched_ctx->sched_policy->pre_exec_hook)
		sched_ctx->sched_policy->pre_exec_hook(task);
}

void _starpu_sched_post_exec_hook(struct starpu_task *task)
{
	struct _starpu_sched_ctx *sched_ctx = _starpu_get_sched_ctx_struct(task->sched_ctx);

#ifdef STARPU_USE_SCHED_CTX_HYPERVISOR
	if(task->hypervisor_tag > 0 && sched_ctx != NULL &&
	   sched_ctx->id != 0 && sched_ctx->perf_counters != NULL)
		sched_ctx->perf_counters->notify_post_exec_hook(sched_ctx->id, task->hypervisor_tag);
#endif //STARPU_USE_SCHED_CTX_HYPERVISOR

	if (sched_ctx->sched_policy->post_exec_hook)
		sched_ctx->sched_policy->post_exec_hook(task);
}

void _starpu_wait_on_sched_event(void)
{
	struct _starpu_worker *worker = _starpu_get_local_worker_key();

	_STARPU_PTHREAD_MUTEX_LOCK(&worker->sched_mutex);

	_starpu_handle_all_pending_node_data_requests(worker->memory_node);

	if (_starpu_machine_is_running())
	{
#ifndef STARPU_NON_BLOCKING_DRIVERS
		_STARPU_PTHREAD_COND_WAIT(worker->sched_cond,
					  worker->sched_mutex);
#endif
	}

	_STARPU_PTHREAD_MUTEX_UNLOCK(&worker->sched_mutex);
}

/* The scheduling policy may put tasks directly into a worker's local queue so
 * that it is not always necessary to create its own queue when the local queue
 * is sufficient. If "back" not null, the task is put at the back of the queue
 * where the worker will pop tasks first. Setting "back" to 0 therefore ensures
 * a FIFO ordering. */
int starpu_push_local_task(int workerid, struct starpu_task *task, int back)
{
	struct _starpu_worker *worker = _starpu_get_worker_struct(workerid);

	int ret =  _starpu_push_local_task(worker, task, back);

	task->scheduled = 1;

	return ret;
}
