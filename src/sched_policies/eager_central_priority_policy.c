/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2010-2016  Université de Bordeaux
 * Copyright (C) 2010, 2011, 2012, 2013, 2015, 2016  CNRS
 * Copyright (C) 2011  INRIA
 * Copyright (C) 2016  Uppsala University
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

/*
 *	This is policy where every worker use the same JOB QUEUE, but taking
 *	task priorities into account
 */

#include <starpu.h>
#include <starpu_scheduler.h>
#include <starpu_bitmap.h>

#include <common/fxt.h>
#include <core/workers.h>

#define DEFAULT_MIN_LEVEL	(-5)
#define DEFAULT_MAX_LEVEL	(+5)

struct _starpu_priority_taskq
{
	int min_prio;
	int max_prio;
	/* the actual lists
	 *	taskq[p] is for priority [p - STARPU_MIN_PRIO] */
	struct starpu_task_list *taskq;
	unsigned *ntasks;

	unsigned total_ntasks;
};

struct _starpu_eager_central_prio_data
{
	struct _starpu_priority_taskq *taskq;
	starpu_pthread_mutex_t policy_mutex;
	struct starpu_bitmap *waiters;
};

/*
 * Centralized queue with priorities
 */

static struct _starpu_priority_taskq *_starpu_create_priority_taskq(int min_prio, int max_prio)
{
	struct _starpu_priority_taskq *central_queue;

	_STARPU_MALLOC(central_queue, sizeof(struct _starpu_priority_taskq));
	central_queue->min_prio = min_prio;
	central_queue->max_prio = max_prio;
	central_queue->total_ntasks = 0;
	_STARPU_MALLOC(central_queue->taskq, (max_prio-min_prio+1) * sizeof(struct starpu_task_list));
	_STARPU_MALLOC(central_queue->ntasks, (max_prio-min_prio+1) * sizeof(unsigned));

	int prio;
	for (prio = 0; prio < (max_prio-min_prio+1); prio++)
	{
		starpu_task_list_init(&central_queue->taskq[prio]);
		central_queue->ntasks[prio] = 0;
	}

	return central_queue;
}

static void _starpu_destroy_priority_taskq(struct _starpu_priority_taskq *priority_queue)
{
	free(priority_queue->ntasks);
	free(priority_queue->taskq);
	free(priority_queue);
}

static void initialize_eager_center_priority_policy(unsigned sched_ctx_id)
{
	struct _starpu_eager_central_prio_data *data;
	_STARPU_MALLOC(data, sizeof(struct _starpu_eager_central_prio_data));

	/* In this policy, we support more than two levels of priority. */

	if (starpu_sched_ctx_min_priority_is_set(sched_ctx_id) == 0)
		starpu_sched_ctx_set_min_priority(sched_ctx_id, DEFAULT_MIN_LEVEL);
	if (starpu_sched_ctx_max_priority_is_set(sched_ctx_id) == 0)
		starpu_sched_ctx_set_max_priority(sched_ctx_id, DEFAULT_MAX_LEVEL);

	/* only a single queue (even though there are several internaly) */
	data->taskq = _starpu_create_priority_taskq(starpu_sched_ctx_get_min_priority(sched_ctx_id), starpu_sched_ctx_get_max_priority(sched_ctx_id));
	data->waiters = starpu_bitmap_create();

	/* Tell helgrind that it's fine to check for empty fifo in
	 * _starpu_priority_pop_task without actual mutex (it's just an
	 * integer) */
	STARPU_HG_DISABLE_CHECKING(data->taskq->total_ntasks);
	starpu_sched_ctx_set_policy_data(sched_ctx_id, (void*)data);
	STARPU_PTHREAD_MUTEX_INIT(&data->policy_mutex, NULL);
}

static void deinitialize_eager_center_priority_policy(unsigned sched_ctx_id)
{
	/* TODO check that there is no task left in the queue */
	struct _starpu_eager_central_prio_data *data = (struct _starpu_eager_central_prio_data*)starpu_sched_ctx_get_policy_data(sched_ctx_id);

	/* deallocate the job queue */
	_starpu_destroy_priority_taskq(data->taskq);
	starpu_bitmap_destroy(data->waiters);

	STARPU_PTHREAD_MUTEX_DESTROY(&data->policy_mutex);
	free(data);
}

static int _starpu_priority_push_task(struct starpu_task *task)
{
	unsigned sched_ctx_id = task->sched_ctx;
	struct _starpu_eager_central_prio_data *data = (struct _starpu_eager_central_prio_data*)starpu_sched_ctx_get_policy_data(sched_ctx_id);
	struct _starpu_priority_taskq *taskq = data->taskq;

	STARPU_PTHREAD_MUTEX_LOCK(&data->policy_mutex);
	unsigned priolevel = task->priority - starpu_sched_ctx_get_min_priority(sched_ctx_id);
	STARPU_ASSERT_MSG(task->priority >= starpu_sched_ctx_get_min_priority(sched_ctx_id) &&
			  task->priority <= starpu_sched_ctx_get_max_priority(sched_ctx_id), "task priority %d is not between minimum %d and maximum %d\n", task->priority, starpu_sched_ctx_get_min_priority(sched_ctx_id), starpu_sched_ctx_get_max_priority(sched_ctx_id));

	starpu_task_list_push_back(&taskq->taskq[priolevel], task);
	taskq->ntasks[priolevel]++;
	taskq->total_ntasks++;
	starpu_push_task_end(task);

	/*if there are no tasks block */
	/* wake people waiting for a task */
	struct starpu_worker_collection *workers = starpu_sched_ctx_get_worker_collection(sched_ctx_id);

	struct starpu_sched_ctx_iterator it;
#ifndef STARPU_NON_BLOCKING_DRIVERS
	char dowake[STARPU_NMAXWORKERS] = { 0 };
#endif

	workers->init_iterator_for_parallel_tasks(workers, &it, task);
	while(workers->has_next(workers, &it))
	{
		unsigned worker = workers->get_next(workers, &it);

#ifdef STARPU_NON_BLOCKING_DRIVERS
		if (!starpu_bitmap_get(data->waiters, worker))
			/* This worker is not waiting for a task */
			continue;
#endif

		if (starpu_worker_can_execute_task_first_impl(worker, task, NULL))
		{
			/* It can execute this one, tell him! */
#ifdef STARPU_NON_BLOCKING_DRIVERS
			starpu_bitmap_unset(data->waiters, worker);
			/* We really woke at least somebody, no need to wake somebody else */
			break;
#else
			dowake[worker] = 1;
#endif
		}
	}
	/* Let the task free */
	STARPU_PTHREAD_MUTEX_UNLOCK(&data->policy_mutex);

#if !defined(STARPU_NON_BLOCKING_DRIVERS) || defined(STARPU_SIMGRID)
	/* Now that we have a list of potential workers, try to wake one */

	workers->init_iterator(workers, &it);
	while(workers->has_next(workers, &it))
	{
		unsigned worker = workers->get_next(workers, &it);
		if (dowake[worker])
			if (_starpu_wake_worker_relax(worker))
				break; // wake up a single worker
	}
#endif

	starpu_sched_ctx_list_task_counters_increment_all(task, sched_ctx_id);

	return 0;
}

static struct starpu_task *_starpu_priority_pop_task(unsigned sched_ctx_id)
{
	struct starpu_task *chosen_task = NULL, *task, *nexttask;
	unsigned workerid = starpu_worker_get_id_check();
	int skipped = 0;

	struct _starpu_eager_central_prio_data *data = (struct _starpu_eager_central_prio_data*)starpu_sched_ctx_get_policy_data(sched_ctx_id);

	struct _starpu_priority_taskq *taskq = data->taskq;

	/* Here helgrind would shout that this is unprotected, this is just an
	 * integer access, and we hold the sched mutex, so we can not miss any
	 * wake up. */
	if (!STARPU_RUNNING_ON_VALGRIND && taskq->total_ntasks == 0)
	{
		return NULL;
	}

#ifdef STARPU_NON_BLOCKING_DRIVERS
	if (!STARPU_RUNNING_ON_VALGRIND && starpu_bitmap_get(data->waiters, workerid))
		/* Nobody woke us, avoid bothering the mutex */
	{
		return NULL;
	}
#endif
	/* block until some event happens */
	_starpu_worker_relax_on();
	STARPU_PTHREAD_MUTEX_LOCK(&data->policy_mutex);
	_starpu_worker_relax_off();

	unsigned priolevel = taskq->max_prio - taskq->min_prio;
	do
	{
		if (taskq->ntasks[priolevel] > 0)
		{
			for (task  = starpu_task_list_begin(&taskq->taskq[priolevel]);
			     task != starpu_task_list_end(&taskq->taskq[priolevel]) && !chosen_task;
			     task  = nexttask)
			{
				unsigned nimpl;
				nexttask = starpu_task_list_next(task);
				if (starpu_worker_can_execute_task_first_impl(workerid, task, &nimpl))
				{
					/* there is some task that we can grab */
					starpu_task_set_implementation(task, nimpl);
					starpu_task_list_erase(&taskq->taskq[priolevel], task);
					chosen_task = task;
					taskq->ntasks[priolevel]--;
					taskq->total_ntasks--;
					break;
				}
				else
					skipped = 1;
			}
		}
	}
	while (!chosen_task && priolevel-- > 0);

	if (!chosen_task && skipped)
	{
		/* Notify another worker to do that task */
		struct starpu_worker_collection *workers = starpu_sched_ctx_get_worker_collection(sched_ctx_id);

		struct starpu_sched_ctx_iterator it;
		workers->init_iterator_for_parallel_tasks(workers, &it, chosen_task);
		while(workers->has_next(workers, &it))
		{
			unsigned worker = workers->get_next(workers, &it);

			if(worker != workerid)
			{
#ifdef STARPU_NON_BLOCKING_DRIVERS
				starpu_bitmap_unset(data->waiters, worker);
#else
				_starpu_wake_worker_relax(worker);
#endif
			}
		}

	}

	if (!chosen_task)
		/* Tell pushers that we are waiting for tasks for us */
		starpu_bitmap_set(data->waiters, workerid);

	STARPU_PTHREAD_MUTEX_UNLOCK(&data->policy_mutex);
	if(chosen_task)
	{
		_starpu_worker_relax_on();
		_starpu_sched_ctx_lock_write(sched_ctx_id);
		_starpu_worker_relax_off();
		starpu_sched_ctx_list_task_counters_decrement_all_ctx_locked(chosen_task, sched_ctx_id);

                unsigned child_sched_ctx = starpu_sched_ctx_worker_is_master_for_child_ctx(workerid, sched_ctx_id);
		if(child_sched_ctx != STARPU_NMAX_SCHED_CTXS)
		{
			starpu_sched_ctx_move_task_to_ctx_locked(chosen_task, child_sched_ctx, 1);
			starpu_sched_ctx_revert_task_counters_ctx_locked(sched_ctx_id, chosen_task->flops);
			chosen_task = NULL;
		}
		_starpu_sched_ctx_unlock_write(sched_ctx_id);
	}

	return chosen_task;
}

static void eager_center_priority_add_workers(unsigned sched_ctx_id, int *workerids, unsigned nworkers)
{
	unsigned i;
        for (i = 0; i < nworkers; i++)
        {
		int workerid = workerids[i];
		int curr_workerid = _starpu_worker_get_id();
		if(workerid != curr_workerid)
			starpu_wake_worker_locked(workerid);

                starpu_sched_ctx_worker_shares_tasks_lists(workerid, sched_ctx_id);
        }
}

struct starpu_sched_policy _starpu_sched_prio_policy =
{
	.add_workers = eager_center_priority_add_workers,
	.init_sched = initialize_eager_center_priority_policy,
	.deinit_sched = deinitialize_eager_center_priority_policy,
	/* we always use priorities in that policy */
	.push_task = _starpu_priority_push_task,
	.pop_task = _starpu_priority_pop_task,
	.pre_exec_hook = NULL,
	.post_exec_hook = NULL,
	.pop_every_task = NULL,
	.policy_name = "prio",
	.policy_description = "eager (with priorities)",
	.worker_type = STARPU_WORKER_LIST,
};
