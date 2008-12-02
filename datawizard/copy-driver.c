#include <pthread.h>
#include <core/policies/sched_policy.h>
#include "copy-driver.h"
#include "memalloc.h"

mem_node_descr descr;
static pthread_key_t memory_node_key;

unsigned register_memory_node(node_kind kind)
{
	unsigned nnodes;
	/* ATOMIC_ADD returns the new value ... */
	nnodes = ATOMIC_ADD(&descr.nnodes, 1);

	descr.nodes[nnodes-1] = kind;

	/* for now, there is no queue related to that newly created node */
	descr.queues_count[nnodes-1] = 0;

	return (nnodes-1);
}


/* TODO move in a more appropriate file */
/* attach a queue to a memory node */
void memory_node_attach_queue(struct jobq_s *q, unsigned nodeid)
{
	unsigned nqueues;
	nqueues = ATOMIC_ADD(&descr.queues_count[nodeid], 1);

	descr.attached_queues[nodeid][nqueues-1] = q;
}

void wake_all_blocked_workers_on_node(unsigned nodeid)
{
	/* wake up all queues on that node */
	unsigned q_id;
	for (q_id = 0; q_id < descr.queues_count[nodeid]; q_id++)
	{
		struct jobq_s *q;
		q  = descr.attached_queues[nodeid][q_id];

		/* wake anybody waiting on that queue */
		pthread_mutex_lock(&q->activity_mutex);
		pthread_cond_broadcast(&q->activity_cond);
		pthread_mutex_unlock(&q->activity_mutex);
	}
}

void wake_all_blocked_workers(void)
{
	/* workers may be blocked on the policy's global condition */
	struct sched_policy_s *sched = get_sched_policy();
	pthread_cond_t *sched_cond = &sched->sched_activity_cond;
	pthread_mutex_t *sched_mutex = &sched->sched_activity_mutex;

	pthread_mutex_lock(sched_mutex);
	pthread_cond_broadcast(sched_cond);
	pthread_mutex_unlock(sched_mutex);

	/* workers may be blocked on the various queues' conditions */
	unsigned node;
	for (node = 0; node < descr.nnodes; node++)
	{
		wake_all_blocked_workers_on_node(node);
	}
}

void init_memory_nodes()
{
	/* there is no node yet, subsequent nodes will be 
	 * added using register_memory_node */
	descr.nnodes = 0;

	pthread_key_create(&memory_node_key, NULL);

	unsigned i;
	for (i = 0; i < MAXNODES; i++) 
	{
		descr.nodes[i] = UNUSED; 
	}

	init_mem_chunk_lists();
	init_data_request_lists();
}

void set_local_memory_node_key(unsigned *node)
{
	pthread_setspecific(memory_node_key, node);
}

unsigned get_local_memory_node(void)
{
	unsigned *memory_node;
	memory_node = pthread_getspecific(memory_node_key);
	
	/* in case this is called by the programmer, we assume the RAM node 
	   is the appropriate memory node ... so we return 0 XXX */
	if (!memory_node)
		return 0;

	return *memory_node;
}

inline node_kind get_node_kind(uint32_t node)
{
	return descr.nodes[node];
}

static int allocate_per_node_buffer(data_state *state, uint32_t node)
{
	int ret;

	if (!state->per_node[node].allocated) {
		/* there is no room available for the data yet */
		ret = allocate_memory_on_node(state, node);
		if (ret == -ENOMEM)
			goto nomem;
	}

	return 0;
nomem:
	/* there was not enough memory to allocate the buffer */
	return -ENOMEM;
}

int __attribute__((warn_unused_result)) driver_copy_data_1_to_1(data_state *state, uint32_t src_node, 
				uint32_t dst_node, unsigned donotread)
{
	int ret;

	/* first make sure the destination has an allocated buffer */
	ret = allocate_per_node_buffer(state, dst_node);
	if (!ret)
		goto nomem;

	/* if there is no need to actually read the data, 
	 * we do not perform any transfer */
	if (!donotread) {
		ASSERT(state->ops);
		ASSERT(state->ops->copy_data_1_to_1);
		state->ops->copy_data_1_to_1(state, src_node, dst_node);
	}

	return 0;

nomem:
	return -ENOMEM;
}

static uint32_t choose_src_node(uint32_t src_node_mask)
{
	unsigned src_node = 0;
	unsigned i;

	/* first find the node that will be the actual source */
	for (i = 0; i < MAXNODES; i++)
	{
		if (src_node_mask & (1<<i))
		{
			/* this is a potential candidate */
			src_node = i;

			/* however GPU are expensive sources, really !
			 * 	other should be ok */
			if (descr.nodes[i] != CUDA_RAM)
				break;

			/* XXX do a better algorithm to distribute the memory copies */
		}
	}

	return src_node;
}

__attribute__((warn_unused_result))
int driver_copy_data(data_state *state, uint32_t src_node_mask,
			 uint32_t dst_node, unsigned donotread)
{
	int ret;
	uint32_t src_node = choose_src_node(src_node_mask);

	/* possibly returns -1 if there was no memory left */
	ret = driver_copy_data_1_to_1(state, src_node, dst_node, donotread);

	return ret;
}
