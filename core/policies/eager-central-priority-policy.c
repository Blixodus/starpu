#include <core/policies/eager-central-priority-policy.h>

/* the former is the actual queue, the latter some container */
static struct jobq_s *jobq;

static void init_priority_queue_design(void)
{
	/* only a single queue (even though there are several internaly) */
	jobq = create_priority_jobq();

	init_priority_queues_mechanisms();

	/* we always use priorities in that policy */
	jobq->push_task = priority_push_task;
	jobq->push_prio_task = priority_push_task;
	jobq->pop_task = priority_pop_task;
}

static struct jobq_s *func_init_priority_queue(void)
{
	return jobq;
}

void initialize_eager_center_priority_policy(struct machine_config_s *config, 
			__attribute__ ((unused))	struct sched_policy_s *_policy) 
{
	setup_queues(init_priority_queue_design, func_init_priority_queue, config);
}

struct jobq_s *get_local_queue_eager_priority(struct sched_policy_s *policy __attribute__ ((unused)))
{
	/* this is trivial for that strategy */
	return jobq;
}


