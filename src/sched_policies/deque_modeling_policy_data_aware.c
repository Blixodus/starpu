/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2010-2012  Université de Bordeaux 1
 * Copyright (C) 2010, 2011, 2012  Centre National de la Recherche Scientifique
 * Copyright (C) 2011  Télécom-SudParis
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

/* Distributed queues using performance modeling to assign tasks */

#include <limits.h>

#include <core/perfmodel/perfmodel.h>
#include <core/task_bundle.h>
#include <core/workers.h>
#include <sched_policies/fifo_queues.h>
#include <core/perfmodel/perfmodel.h>
#include <starpu_parameters.h>

#ifndef DBL_MIN
#define DBL_MIN __DBL_MIN__
#endif

#ifndef DBL_MAX
#define DBL_MAX __DBL_MAX__
#endif

typedef struct {
	double alpha;
	double beta;
	double _gamma;
	double idle_power;

	struct _starpu_fifo_taskq **queue_array;

	long int total_task_cnt;
	long int ready_task_cnt;
} dmda_data;


static int count_non_ready_buffers(struct starpu_task *task, uint32_t node)
{
	int cnt = 0;
	unsigned nbuffers = task->cl->nbuffers;
	unsigned index;

	for (index = 0; index < nbuffers; index++)
	{
		starpu_data_handle_t handle;

		handle = task->handles[index];

		int is_valid;
		starpu_data_query_status(handle, node, NULL, &is_valid, NULL);

		if (!is_valid)
			cnt++;
	}

	return cnt;
}

static struct starpu_task *_starpu_fifo_pop_first_ready_task(struct _starpu_fifo_taskq *fifo_queue, unsigned node)
{
	struct starpu_task *task = NULL, *current;

	if (fifo_queue->ntasks == 0)
		return NULL;

	if (fifo_queue->ntasks > 0)
	{
		fifo_queue->ntasks--;

		task = starpu_task_list_back(&fifo_queue->taskq);

		int first_task_priority = task->priority;

		current = task;

		int non_ready_best = INT_MAX;

		while (current)
		{
			int priority = current->priority;

			if (priority <= first_task_priority)
			{
				int non_ready = count_non_ready_buffers(current, node);
				if (non_ready < non_ready_best)
				{
					non_ready_best = non_ready;
					task = current;

					if (non_ready == 0)
						break;
				}
			}

			current = current->prev;
		}

		starpu_task_list_erase(&fifo_queue->taskq, task);

		_STARPU_TRACE_JOB_POP(task, 0);
	}

	return task;
}

static struct starpu_task *dmda_pop_ready_task(unsigned sched_ctx_id)
{
	dmda_data *dt = (dmda_data*)starpu_get_sched_ctx_policy_data(sched_ctx_id);

	struct starpu_task *task;

	int workerid = starpu_worker_get_id();
	struct _starpu_fifo_taskq *fifo = dt->queue_array[workerid];

	unsigned node = starpu_worker_get_memory_node(workerid);

	task = _starpu_fifo_pop_first_ready_task(fifo, node);
	if (task)
	{
		double model = task->predicted;

		fifo->exp_len -= model;
		fifo->exp_start = starpu_timing_now() + model;
		fifo->exp_end = fifo->exp_start + fifo->exp_len;

#ifdef STARPU_VERBOSE
		if (task->cl)
		{
			int non_ready = count_non_ready_buffers(task, starpu_worker_get_memory_node(workerid));
			if (non_ready == 0)
				dt->ready_task_cnt++;
		}

		dt->total_task_cnt++;
#endif
	}

	return task;
}

static struct starpu_task *dmda_pop_task(unsigned sched_ctx_id)
{
	dmda_data *dt = (dmda_data*)starpu_get_sched_ctx_policy_data(sched_ctx_id);

	struct starpu_task *task;

	int workerid = starpu_worker_get_id();
	struct _starpu_fifo_taskq *fifo = dt->queue_array[workerid];

	task = _starpu_fifo_pop_local_task(fifo);
	if (task)
	{
		double model = task->predicted;

		fifo->exp_len -= model;
		fifo->exp_start = starpu_timing_now() + model;
		fifo->exp_end = fifo->exp_start + fifo->exp_len;

#ifdef STARPU_VERBOSE
		if (task->cl)
		{
			int non_ready = count_non_ready_buffers(task, starpu_worker_get_memory_node(workerid));
			if (non_ready == 0)
				dt->ready_task_cnt++;
		}

		dt->total_task_cnt++;
#endif
	}

	return task;
}



static struct starpu_task *dmda_pop_every_task(unsigned sched_ctx_id)
{
	dmda_data *dt = (dmda_data*)starpu_get_sched_ctx_policy_data(sched_ctx_id);

	struct starpu_task *new_list;

	int workerid = starpu_worker_get_id();
	struct _starpu_fifo_taskq *fifo = dt->queue_array[workerid];

	pthread_mutex_t *sched_mutex;
	pthread_cond_t *sched_cond;
	starpu_worker_get_sched_condition(sched_ctx_id, workerid, &sched_mutex, &sched_cond);
	new_list = _starpu_fifo_pop_every_task(fifo, sched_mutex, workerid);

	while (new_list)
	{
		double model = new_list->predicted;

		fifo->exp_len -= model;
		fifo->exp_start = starpu_timing_now() + model;
		fifo->exp_end = fifo->exp_start + fifo->exp_len;

		new_list = new_list->next;
	}

	return new_list;
}




static int push_task_on_best_worker(struct starpu_task *task, int best_workerid, double predicted, int prio, unsigned sched_ctx_id)
{
	dmda_data *dt = (dmda_data*)starpu_get_sched_ctx_policy_data(sched_ctx_id);
	/* make sure someone coule execute that task ! */
	STARPU_ASSERT(best_workerid != -1);

	struct _starpu_fifo_taskq *fifo;
	fifo = dt->queue_array[best_workerid];

	fifo->exp_end += predicted;
	fifo->exp_len += predicted;

	task->predicted = predicted;

	/* TODO predicted_transfer */

	unsigned memory_node = starpu_worker_get_memory_node(best_workerid);

	if (starpu_get_prefetch_flag())
		starpu_prefetch_task_input_on_node(task, memory_node);

	pthread_mutex_t *sched_mutex;
	pthread_cond_t *sched_cond;
	starpu_worker_get_sched_condition(sched_ctx_id, best_workerid, &sched_mutex, &sched_cond);

#ifdef STARPU_USE_SCHED_CTX_HYPERVISOR
        starpu_call_pushed_task_cb(best_workerid, sched_ctx_id);
#endif //STARPU_USE_SCHED_CTX_HYPERVISOR 

	if (prio)
		return _starpu_fifo_push_sorted_task(dt->queue_array[best_workerid],
			sched_mutex, sched_cond, task);
	else
		return _starpu_fifo_push_task(dt->queue_array[best_workerid],
			sched_mutex, sched_cond, task);
}

/* TODO: factorize with dmda!! */
static int _dm_push_task(struct starpu_task *task, unsigned prio, unsigned sched_ctx_id)
{
	dmda_data *dt = (dmda_data*)starpu_get_sched_ctx_policy_data(sched_ctx_id);
	/* find the queue */
	struct _starpu_fifo_taskq *fifo;
	unsigned worker, worker_ctx;
	int best = -1;

	double best_exp_end = 0.0;
	double model_best = 0.0;

	int ntasks_best = -1;
	double ntasks_best_end = 0.0;
	int calibrating = 0;

	/* A priori, we know all estimations */
	int unknown = 0;

	unsigned best_impl = 0;
	unsigned nimpl;
	struct worker_collection *workers = starpu_get_worker_collection_of_sched_ctx(sched_ctx_id);

	if(workers->init_cursor)
                workers->init_cursor(workers);

	while(workers->has_next(workers))
        {
                worker = workers->get_next(workers);
		for (nimpl = 0; nimpl < STARPU_MAXIMPLEMENTATIONS; nimpl++)
		{
			double exp_end;
		
			fifo = dt->queue_array[worker];

			/* Sometimes workers didn't take the tasks as early as we expected */
			fifo->exp_start = STARPU_MAX(fifo->exp_start, starpu_timing_now());
			fifo->exp_end = fifo->exp_start + fifo->exp_len;

			if (!starpu_worker_can_execute_task(worker, task, nimpl))
			{
				/* no one on that queue may execute this task */
				worker_ctx++;
				continue;
			}

			enum starpu_perf_archtype perf_arch = starpu_worker_get_perf_archtype(worker);
			double local_length = starpu_task_expected_length(task, perf_arch, nimpl);
			double ntasks_end = fifo->ntasks / starpu_worker_get_relative_speedup(perf_arch);

			//_STARPU_DEBUG("Scheduler dm: task length (%lf) worker (%u) kernel (%u) \n", local_length,worker,nimpl);

			if (ntasks_best == -1
			    || (!calibrating && ntasks_end < ntasks_best_end) /* Not calibrating, take better task */
			    || (!calibrating && isnan(local_length)) /* Not calibrating but this worker is being calibrated */
			    || (calibrating && isnan(local_length) && ntasks_end < ntasks_best_end) /* Calibrating, compete this worker with other non-calibrated */
				)
			{
				ntasks_best_end = ntasks_end;
				ntasks_best = worker;
				best_impl = nimpl;
			}

			if (isnan(local_length))
				/* we are calibrating, we want to speed-up calibration time
				 * so we privilege non-calibrated tasks (but still
				 * greedily distribute them to avoid dumb schedules) */
				calibrating = 1;

			if (isnan(local_length) || _STARPU_IS_ZERO(local_length))
				/* there is no prediction available for that task
				 * with that arch yet, so switch to a greedy strategy */
				unknown = 1;

			if (unknown)
				continue;

			exp_end = fifo->exp_start + fifo->exp_len + local_length;

			if (best == -1 || exp_end < best_exp_end)
			{
				/* a better solution was found */
				best_exp_end = exp_end;
				best = worker;
				model_best = local_length;
				best_impl = nimpl;
			}
		}
		worker_ctx++;
	}

	if (unknown)
	{
		best = ntasks_best;
		model_best = 0.0;
	}

	//_STARPU_DEBUG("Scheduler dm: kernel (%u)\n", best_impl);

	if(workers->init_cursor)
                workers->deinit_cursor(workers);

	 _starpu_get_job_associated_to_task(task)->nimpl = best_impl;

	/* we should now have the best worker in variable "best" */
	return push_task_on_best_worker(task, best, model_best, prio, sched_ctx_id);
}

static void compute_all_performance_predictions(struct starpu_task *task,
					double local_task_length[STARPU_NMAXWORKERS][STARPU_MAXIMPLEMENTATIONS],
					double exp_end[STARPU_NMAXWORKERS][STARPU_MAXIMPLEMENTATIONS],
					double *max_exp_endp,
					double *best_exp_endp,
					double local_data_penalty[STARPU_NMAXWORKERS][STARPU_MAXIMPLEMENTATIONS],
					double local_power[STARPU_NMAXWORKERS][STARPU_MAXIMPLEMENTATIONS],
					int *forced_worker, int *forced_impl, unsigned sched_ctx_id)
{
	int calibrating = 0;
	double max_exp_end = DBL_MIN;
	double best_exp_end = DBL_MAX;
	int ntasks_best = -1;
	int nimpl_best = 0;
	double ntasks_best_end = 0.0;

	/* A priori, we know all estimations */
	int unknown = 0;
	unsigned worker, worker_ctx = 0;
	
	unsigned nimpl;
	
	starpu_task_bundle_t bundle = task->bundle;
	dmda_data *dt = (dmda_data*)starpu_get_sched_ctx_policy_data(sched_ctx_id);
	struct worker_collection *workers = starpu_get_worker_collection_of_sched_ctx(sched_ctx_id);
	
	/* find the queue */
	struct _starpu_fifo_taskq *fifo;
	
	while(workers->has_next(workers))
	{
		worker = workers->get_next(workers);
		fifo = dt->queue_array[worker];
		for(nimpl  = 0; nimpl < STARPU_MAXIMPLEMENTATIONS; nimpl++)
	 	{
			if (!starpu_worker_can_execute_task(worker, task, nimpl))
			{
				/* no one on that queue may execute this task */
				continue;
			}
			
			/* Sometimes workers didn't take the tasks as early as we expected */
			fifo->exp_start = STARPU_MAX(fifo->exp_start, starpu_timing_now());
			exp_end[worker_ctx][nimpl] = fifo->exp_start + fifo->exp_len;
			if (exp_end[worker_ctx][nimpl] > max_exp_end)
				max_exp_end = exp_end[worker_ctx][nimpl];
			
			enum starpu_perf_archtype perf_arch = starpu_worker_get_perf_archtype(worker);
			unsigned memory_node = starpu_worker_get_memory_node(worker);
			
			//_STARPU_DEBUG("Scheduler dmda: task length (%lf) worker (%u) kernel (%u) \n", local_task_length[worker][nimpl],worker,nimpl);
			
			if (bundle)
			{
				STARPU_ABORT(); /* Not implemented yet. */
			}
			else
			{
				local_task_length[worker_ctx][nimpl] = starpu_task_expected_length(task, perf_arch, nimpl);
				local_data_penalty[worker_ctx][nimpl] = starpu_task_expected_data_transfer_time(memory_node, task);
				local_power[worker_ctx][nimpl] = starpu_task_expected_power(task, perf_arch,nimpl);
			}
			
			double ntasks_end = fifo->ntasks / starpu_worker_get_relative_speedup(perf_arch);
			
			if (ntasks_best == -1
			    || (!calibrating && ntasks_end < ntasks_best_end) /* Not calibrating, take better worker */
			    || (!calibrating && isnan(local_task_length[worker_ctx][nimpl])) /* Not calibrating but this worker is being calibrated */
			    || (calibrating && isnan(local_task_length[worker_ctx][nimpl]) && ntasks_end < ntasks_best_end) /* Calibrating, compete this worker with other non-calibrated */
				)
			{
				ntasks_best_end = ntasks_end;
				ntasks_best = worker;
				nimpl_best = nimpl;
			}

			if (isnan(local_task_length[worker_ctx][nimpl]))
				/* we are calibrating, we want to speed-up calibration time
				 * so we privilege non-calibrated tasks (but still
				 * greedily distribute them to avoid dumb schedules) */
				calibrating = 1;
			
			if (isnan(local_task_length[worker_ctx][nimpl])
					|| _STARPU_IS_ZERO(local_task_length[worker_ctx][nimpl]))
				/* there is no prediction available for that task
				 * with that arch (yet or at all), so switch to a greedy strategy */
				unknown = 1;
			
			if (unknown)
				continue;
			
			exp_end[worker_ctx][nimpl] = fifo->exp_start + fifo->exp_len + local_task_length[worker_ctx][nimpl];
			
			if (exp_end[worker_ctx][nimpl] < best_exp_end)
			{
				/* a better solution was found */
				best_exp_end = exp_end[worker_ctx][nimpl];
				nimpl_best = nimpl;
			}
			
			if (isnan(local_power[worker_ctx][nimpl]))
				local_power[worker_ctx][nimpl] = 0.;
			
		}
		worker_ctx++;
	}

	*forced_worker = unknown?ntasks_best:-1;
	*forced_impl = unknown?nimpl_best:-1;

	*best_exp_endp = best_exp_end;
	*max_exp_endp = max_exp_end;
}

static int _dmda_push_task(struct starpu_task *task, unsigned prio, unsigned sched_ctx_id)
{
	/* find the queue */
	unsigned worker, worker_ctx = 0;
	int best = -1, best_in_ctx = -1;
	int selected_impl = 0;
	double model_best = 0.0;

	/* this flag is set if the corresponding worker is selected because
	   there is no performance prediction available yet */
	int forced_best = -1;
	int forced_impl = -1;

	dmda_data *dt = (dmda_data*)starpu_get_sched_ctx_policy_data(sched_ctx_id);
	struct worker_collection *workers = starpu_get_worker_collection_of_sched_ctx(sched_ctx_id);
	unsigned nworkers_ctx = workers->nworkers;
	double local_task_length[STARPU_NMAXWORKERS][STARPU_MAXIMPLEMENTATIONS];
	double local_data_penalty[STARPU_NMAXWORKERS][STARPU_MAXIMPLEMENTATIONS];
	double local_power[STARPU_NMAXWORKERS][STARPU_MAXIMPLEMENTATIONS];
	double exp_end[STARPU_NMAXWORKERS][STARPU_MAXIMPLEMENTATIONS];
	double max_exp_end = 0.0;
	double best_exp_end;

	double fitness[nworkers_ctx][STARPU_MAXIMPLEMENTATIONS];

	if(workers->init_cursor)
		workers->init_cursor(workers);

	compute_all_performance_predictions(task,
										local_task_length,
										exp_end,
										&max_exp_end,
										&best_exp_end,
										local_data_penalty,
										local_power,
										&forced_best,
										&forced_impl, sched_ctx_id);

	double best_fitness = -1;

	unsigned nimpl;
	if (forced_best == -1)
	{
		while(workers->has_next(workers))
		{
			worker = workers->get_next(workers);
			for (nimpl = 0; nimpl < STARPU_MAXIMPLEMENTATIONS; nimpl++)
			{
				if (!starpu_worker_can_execute_task(worker, task, nimpl))
				{
					/* no one on that queue may execute this task */
					continue;
				}
				
				
				fitness[worker_ctx][nimpl] = dt->alpha*(exp_end[worker_ctx][nimpl] - best_exp_end) 
					+ dt->beta*(local_data_penalty[worker_ctx][nimpl])
					+ dt->_gamma*(local_power[worker_ctx][nimpl]);
				
				if (exp_end[worker_ctx][nimpl] > max_exp_end)
					/* This placement will make the computation
					 * longer, take into account the idle
					 * consumption of other cpus */
					fitness[worker_ctx][nimpl] += dt->_gamma * dt->idle_power * (exp_end[worker_ctx][nimpl] - max_exp_end) / 1000000.0;
				
				if (best == -1 || fitness[worker_ctx][nimpl] < best_fitness)
				{
					/* we found a better solution */
					best_fitness = fitness[worker_ctx][nimpl];
					best = worker;
					best_in_ctx = worker_ctx;
					selected_impl = nimpl;

					//_STARPU_DEBUG("best fitness (worker %d) %e = alpha*(%e) + beta(%e) +gamma(%e)\n", worker, best_fitness, exp_end[worker][nimpl] - best_exp_end, local_data_penalty[worker][nimpl], local_power[worker][nimpl]);
				}
			}
		}
	}

	STARPU_ASSERT(forced_best != -1 || best != -1);

	if (forced_best != -1)
	{
		/* there is no prediction available for that task
		 * with that arch we want to speed-up calibration time
		 * so we force this measurement */
		best = forced_best;
		model_best = 0.0;
		//penality_best = 0.0;
	}
	else
	{
		model_best = local_task_length[best_in_ctx][selected_impl];
		//penality_best = local_data_penalty[best_in_ctx][best_impl];
	}

        if(workers->init_cursor)
                workers->deinit_cursor(workers);

	//_STARPU_DEBUG("Scheduler dmda: kernel (%u)\n", best_impl);
	 _starpu_get_job_associated_to_task(task)->nimpl = selected_impl;

	/* we should now have the best worker in variable "best" */
	return push_task_on_best_worker(task, best, model_best, prio, sched_ctx_id);
}

static int dmda_push_sorted_task(struct starpu_task *task)
{
	unsigned sched_ctx_id = task->sched_ctx;
        pthread_mutex_t *changing_ctx_mutex = starpu_get_changing_ctx_mutex(sched_ctx_id);
        unsigned nworkers;
        int ret_val = -1;

	_STARPU_PTHREAD_MUTEX_LOCK(changing_ctx_mutex);
	nworkers = starpu_get_nworkers_of_sched_ctx(sched_ctx_id);
	if(nworkers == 0)
	{
		_STARPU_PTHREAD_MUTEX_UNLOCK(changing_ctx_mutex);
		return ret_val;
	}

	ret_val = _dmda_push_task(task, 1, sched_ctx_id);
	_STARPU_PTHREAD_MUTEX_UNLOCK(changing_ctx_mutex);
	return ret_val;

}

static int dm_push_task(struct starpu_task *task)
{
	unsigned sched_ctx_id = task->sched_ctx;
        pthread_mutex_t *changing_ctx_mutex = starpu_get_changing_ctx_mutex(sched_ctx_id);
        unsigned nworkers;
        int ret_val = -1;

	_STARPU_PTHREAD_MUTEX_LOCK(changing_ctx_mutex);
	nworkers = starpu_get_nworkers_of_sched_ctx(sched_ctx_id);
	if(nworkers == 0)
	{
		_STARPU_PTHREAD_MUTEX_UNLOCK(changing_ctx_mutex);
		return ret_val;
	}

	ret_val = _dm_push_task(task, 0, sched_ctx_id);
	_STARPU_PTHREAD_MUTEX_UNLOCK(changing_ctx_mutex);
	return ret_val;
}

static int dmda_push_task(struct starpu_task *task)
{
	unsigned sched_ctx_id = task->sched_ctx;
        pthread_mutex_t *changing_ctx_mutex = starpu_get_changing_ctx_mutex(sched_ctx_id);
        unsigned nworkers;
        int ret_val = -1;

	_STARPU_PTHREAD_MUTEX_LOCK(changing_ctx_mutex);
	nworkers = starpu_get_nworkers_of_sched_ctx(sched_ctx_id);
	if(nworkers == 0)
	{
		_STARPU_PTHREAD_MUTEX_UNLOCK(changing_ctx_mutex);
		return ret_val;
	}

	STARPU_ASSERT(task);
	ret_val = _dmda_push_task(task, 0, sched_ctx_id);
	_STARPU_PTHREAD_MUTEX_UNLOCK(changing_ctx_mutex);
	return ret_val;
}

static void dmda_add_workers(unsigned sched_ctx_id, int *workerids, unsigned nworkers) 
{
	dmda_data *dt = (dmda_data*)starpu_get_sched_ctx_policy_data(sched_ctx_id);

	int workerid;
	unsigned i;
	for (i = 0; i < nworkers; i++)
	{
		workerid = workerids[i];
		dt->queue_array[workerid] = _starpu_create_fifo();
		starpu_worker_init_sched_condition(sched_ctx_id, workerid);
	}
}

static void dmda_remove_workers(unsigned sched_ctx_id, int *workerids, unsigned nworkers)
{
	dmda_data *dt = (dmda_data*)starpu_get_sched_ctx_policy_data(sched_ctx_id);

	int workerid;
	unsigned i;
	for (i = 0; i < nworkers; i++)
	{
		workerid = workerids[i];
		_starpu_destroy_fifo(dt->queue_array[workerid]);
		starpu_worker_deinit_sched_condition(sched_ctx_id, workerid);
	}
}

static void initialize_dmda_policy(unsigned sched_ctx_id) 
{
	starpu_create_worker_collection_for_sched_ctx(sched_ctx_id, WORKER_LIST);

	dmda_data *dt = (dmda_data*)malloc(sizeof(dmda_data));
	dt->alpha = _STARPU_DEFAULT_ALPHA;
	dt->beta = _STARPU_DEFAULT_BETA;
	dt->_gamma = _STARPU_DEFAULT_GAMMA;
	dt->idle_power = 0.0;

	starpu_set_sched_ctx_policy_data(sched_ctx_id, (void*)dt);

	dt->queue_array = (struct _starpu_fifo_taskq**)malloc(STARPU_NMAXWORKERS*sizeof(struct _starpu_fifo_taskq*));

	const char *strval_alpha = getenv("STARPU_SCHED_ALPHA");
	if (strval_alpha)
		dt->alpha = atof(strval_alpha);

	const char *strval_beta = getenv("STARPU_SCHED_BETA");
	if (strval_beta)
		dt->beta = atof(strval_beta);

	const char *strval_gamma = getenv("STARPU_SCHED_GAMMA");
	if (strval_gamma)
		dt->_gamma = atof(strval_gamma);	

	const char *strval_idle_power = getenv("STARPU_IDLE_POWER");
	if (strval_idle_power)
		dt->idle_power = atof(strval_idle_power);

}

static void initialize_dmda_sorted_policy(unsigned sched_ctx_id)
{
	initialize_dmda_policy(sched_ctx_id);

	/* The application may use any integer */
	starpu_sched_set_min_priority(INT_MIN);
	starpu_sched_set_max_priority(INT_MAX);
}

static void deinitialize_dmda_policy(unsigned sched_ctx_id) 
{
	dmda_data *dt = (dmda_data*)starpu_get_sched_ctx_policy_data(sched_ctx_id);
	free(dt->queue_array);
	free(dt);
	starpu_delete_worker_collection_for_sched_ctx(sched_ctx_id);

	_STARPU_DEBUG("total_task_cnt %ld ready_task_cnt %ld -> %f\n", dt->total_task_cnt, dt->ready_task_cnt, (100.0f*dt->ready_task_cnt)/dt->total_task_cnt);
}

/* TODO: use post_exec_hook to fix the expected start */
struct starpu_sched_policy _starpu_sched_dm_policy =
{
	.init_sched = initialize_dmda_policy,
	.deinit_sched = deinitialize_dmda_policy,
	.add_workers = dmda_add_workers ,
        .remove_workers = dmda_remove_workers,
	.push_task = dm_push_task,
	.pop_task = dmda_pop_task,
	.pre_exec_hook = NULL,
	.post_exec_hook = NULL,
	.pop_every_task = dmda_pop_every_task,
	.policy_name = "dm",
	.policy_description = "performance model"
};

struct starpu_sched_policy _starpu_sched_dmda_policy =
{
	.init_sched = initialize_dmda_policy,
	.deinit_sched = deinitialize_dmda_policy,
	.add_workers = dmda_add_workers ,
        .remove_workers = dmda_remove_workers,
	.push_task = dmda_push_task,
	.pop_task = dmda_pop_task,
	.pre_exec_hook = NULL,
	.post_exec_hook = NULL,
	.pop_every_task = dmda_pop_every_task,
	.policy_name = "dmda",
	.policy_description = "data-aware performance model"
};

struct starpu_sched_policy _starpu_sched_dmda_sorted_policy =
{
	.init_sched = initialize_dmda_sorted_policy,
	.deinit_sched = deinitialize_dmda_policy,
	.add_workers = dmda_add_workers ,
        .remove_workers = dmda_remove_workers,
	.push_task = dmda_push_sorted_task,
	.pop_task = dmda_pop_ready_task,
	.pre_exec_hook = NULL,
	.post_exec_hook = NULL,
	.pop_every_task = dmda_pop_every_task,
	.policy_name = "dmdas",
	.policy_description = "data-aware performance model (sorted)"
};

struct starpu_sched_policy _starpu_sched_dmda_ready_policy =
{
	.init_sched = initialize_dmda_policy,
	.deinit_sched = deinitialize_dmda_policy,
	.add_workers = dmda_add_workers ,
        .remove_workers = dmda_remove_workers,
	.push_task = dmda_push_task,
	.pop_task = dmda_pop_ready_task,
	.pre_exec_hook = NULL,
	.post_exec_hook = NULL,
	.pop_every_task = dmda_pop_every_task,
	.policy_name = "dmdar",
	.policy_description = "data-aware performance model (ready)"
};
