/*
 * StarPU
 * Copyright (C) INRIA 2008-2009 (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU Lesser General Public License in COPYING.LGPL for more details.
 */

#include <common/config.h>
#include <core/policies/deque-modeling-policy.h>
#include <core/perfmodel/perfmodel.h>

static unsigned nworkers;
static struct jobq_s *queue_array[STARPU_NMAXWORKERS];
static int use_prefetch = 0;

static starpu_job_t dm_pop_task(struct jobq_s *q)
{
	struct starpu_job_s *j;

	j = _starpu_fifo_pop_task(q);
	if (j) {
		struct starpu_fifo_jobq_s *fifo = q->queue;
		double model = j->predicted;
	
		fifo->exp_len -= model;
		fifo->exp_start = timing_now() + model;
		fifo->exp_end = fifo->exp_start + fifo->exp_len;
	}	

	return j;
}

static struct starpu_job_list_s *dm_pop_every_task(struct jobq_s *q, uint32_t where)
{
	struct starpu_job_list_s *new_list;

	new_list = _starpu_fifo_pop_every_task(q, where);
	if (new_list) {
		starpu_job_itor_t i;
		for(i = starpu_job_list_begin(new_list);
			i != starpu_job_list_end(new_list);
			i = starpu_job_list_next(i))
		{
			struct starpu_fifo_jobq_s *fifo = q->queue;
			double model = i->predicted;
	
			fifo->exp_len -= model;
			fifo->exp_start = timing_now() + model;
			fifo->exp_end = fifo->exp_start + fifo->exp_len;
		}
	}

	return new_list;
}



static int _dm_push_task(struct jobq_s *q __attribute__ ((unused)), starpu_job_t j, unsigned prio)
{
	/* find the queue */
	struct starpu_fifo_jobq_s *fifo;
	unsigned worker;
	int best = -1;

	double best_exp_end = 0.0;
	double model_best = 0.0;

	struct starpu_task *task = j->task;

	for (worker = 0; worker < nworkers; worker++)
	{
		double exp_end;
		
		fifo = queue_array[worker]->queue;

		fifo->exp_start = STARPU_MAX(fifo->exp_start, timing_now());
		fifo->exp_end = STARPU_MAX(fifo->exp_end, timing_now());

		if ((queue_array[worker]->who & task->cl->where) == 0)
		{
			/* no one on that queue may execute this task */
			continue;
		}

		double local_length = _starpu_job_expected_length(queue_array[worker]->who, j, queue_array[worker]->arch);

		if (local_length == -1.0) 
		{
			/* there is no prediction available for that task
			 * with that arch we want to speed-up calibration time 
			 * so we force this measurement */
			/* XXX assert we are benchmarking ! */
			best = worker;
			model_best = 0.0;
			exp_end = fifo->exp_start + fifo->exp_len;
			break;
		}


		exp_end = fifo->exp_start + fifo->exp_len + local_length;

		if (best == -1 || exp_end < best_exp_end)
		{
			/* a better solution was found */
			best_exp_end = exp_end;
			best = worker;
			model_best = local_length;
		}
	}

	
	/* make sure someone coule execute that task ! */
	STARPU_ASSERT(best != -1);

	/* we should now have the best worker in variable "best" */
	fifo = queue_array[best]->queue;

	fifo->exp_end += model_best;
	fifo->exp_len += model_best;

	j->predicted = model_best;

	if (use_prefetch)
		_starpu_prefetch_task_input_on_node(task, queue_array[best]->memory_node);

	if (prio) {
		return _starpu_fifo_push_prio_task(queue_array[best], j);
	} else {
		return _starpu_fifo_push_task(queue_array[best], j);
	}
}

static int dm_push_prio_task(struct jobq_s *q, starpu_job_t j)
{
	return _dm_push_task(q, j, 1);
}

static int dm_push_task(struct jobq_s *q, starpu_job_t j)
{
	if (j->task->priority == STARPU_MAX_PRIO)
		return _dm_push_task(q, j, 1);

	return _dm_push_task(q, j, 0);
}

static struct jobq_s *init_dm_fifo(void)
{
	struct jobq_s *q;

	q = _starpu_create_fifo();

	q->_starpu_push_task = dm_push_task; 
	q->push_prio_task = dm_push_prio_task; 
	q->_starpu_pop_task = dm_pop_task;
	q->_starpu_pop_every_task = dm_pop_every_task;
	q->who = 0;

	queue_array[nworkers++] = q;

	return q;
}

static void initialize_dm_policy(struct starpu_machine_config_s *config, 
	 __attribute__ ((unused)) struct starpu_sched_policy_s *_policy) 
{
	nworkers = 0;

	use_prefetch = starpu_get_env_number("STARPU_PREFETCH");
	if (use_prefetch == -1)
		use_prefetch = 0;

#ifdef STARPU_VERBOSE
	fprintf(stderr, "Using prefetch ? %s\n", use_prefetch?"yes":"no");
#endif

	setup_queues(_starpu_init_fifo_queues_mechanisms, init_dm_fifo, config);
}

static struct jobq_s *get_local_queue_dm(struct starpu_sched_policy_s *policy __attribute__ ((unused)))
{
	struct jobq_s *queue;
	queue = pthread_getspecific(policy->local_queue_key);

	if (!queue)
	{
		/* take one randomly as this *must* be for a push anyway XXX */
		queue = queue_array[0];
	}

	return queue;
}

struct starpu_sched_policy_s sched_dm_policy = {
	.init_sched = initialize_dm_policy,
	.deinit_sched = NULL,
	.get_local_queue = get_local_queue_dm,
	.policy_name = "dm",
	.policy_description = "performance model"
};
