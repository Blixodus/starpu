/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2010-2012  INRIA
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

/* #include <sched_ctx_hypervisor.h> */
/* #include <pthread.h> */

#include "policy_tools.h"

static int _compute_priority(unsigned sched_ctx)
{
	struct policy_config *config = sched_ctx_hypervisor_get_config(sched_ctx);

	int total_priority = 0;

	struct worker_collection *workers = starpu_get_worker_collection_of_sched_ctx(sched_ctx);
	int worker;

	if(workers->init_cursor)
		workers->init_cursor(workers);

	while(workers->has_next(workers))
	{
		worker = workers->get_next(workers);
		total_priority += config->priority[worker];
	}

	if(workers->init_cursor)
		workers->deinit_cursor(workers);
	return total_priority;
}

/* find the context with the slowest priority */
unsigned _find_poor_sched_ctx(unsigned req_sched_ctx, int nworkers_to_move)
{
	int i;
	int highest_priority = -1;
	int current_priority = 0;
	unsigned sched_ctx = STARPU_NMAX_SCHED_CTXS;
	int *sched_ctxs = sched_ctx_hypervisor_get_sched_ctxs();
	int nsched_ctxs = sched_ctx_hypervisor_get_nsched_ctxs();


	struct policy_config *config = NULL;

	for(i = 0; i < nsched_ctxs; i++)
	{
		if(sched_ctxs[i] != STARPU_NMAX_SCHED_CTXS && sched_ctxs[i] != req_sched_ctx)
		{
			unsigned nworkers = starpu_get_nworkers_of_sched_ctx(sched_ctxs[i]);
			config  = sched_ctx_hypervisor_get_config(sched_ctxs[i]);
			if((nworkers + nworkers_to_move) <= config->max_nworkers)
			{
				current_priority = _compute_priority(sched_ctxs[i]);
				if (highest_priority < current_priority)
				{
					highest_priority = current_priority;
					sched_ctx = sched_ctxs[i];
				}
			}
		}
	}

	return sched_ctx;
}

int* _get_first_workers_in_list(int *workers, int nall_workers,  unsigned *nworkers, enum starpu_archtype arch)
{
	int *curr_workers = (int*)malloc((*nworkers)*sizeof(int));

	int w, worker;
	int nfound_workers = 0;
	for(w = 0; w < nall_workers; w++)
	{
		worker = workers == NULL ? w : workers[w];
		enum starpu_archtype curr_arch = starpu_worker_get_type(worker);
		if(arch == STARPU_ANY_WORKER || curr_arch == arch)
		{
			curr_workers[nfound_workers++] = worker;
		}
		if(nfound_workers == *nworkers)
			break;
	}
	if(nfound_workers < *nworkers)
		*nworkers = nfound_workers;
	return curr_workers;
}

/* get first nworkers with the highest idle time in the context */
int* _get_first_workers(unsigned sched_ctx, int *nworkers, enum starpu_archtype arch)
{
	struct sched_ctx_wrapper* sc_w = sched_ctx_hypervisor_get_wrapper(sched_ctx);
	struct policy_config *config = sched_ctx_hypervisor_get_config(sched_ctx);

	int *curr_workers = (int*)malloc((*nworkers) * sizeof(int));
	int i;
	for(i = 0; i < *nworkers; i++)
		curr_workers[i] = -1;

	struct worker_collection *workers = starpu_get_worker_collection_of_sched_ctx(sched_ctx);
	int index;
	int worker;
	int considered = 0;

	if(workers->init_cursor)
		workers->init_cursor(workers);

	for(index = 0; index < *nworkers; index++)
	{
		while(workers->has_next(workers))
		{
			considered = 0;
			worker = workers->get_next(workers);
			enum starpu_archtype curr_arch = starpu_worker_get_type(worker);
			if(arch == STARPU_ANY_WORKER || curr_arch == arch)
			{

				if(!config->fixed_workers[worker])
				{
					for(i = 0; i < index; i++)
					{
						if(curr_workers[i] == worker)
						{
							considered = 1;
							break;
						}
					}

					if(!considered)
					{
						/* the first iteration*/
						if(curr_workers[index] < 0)
						curr_workers[index] = worker;
						/* small priority worker is the first to leave the ctx*/
						else if(config->priority[worker] <
							config->priority[curr_workers[index]])
						curr_workers[index] = worker;
						/* if we don't consider priorities check for the workers
						   with the biggest idle time */
						else if(config->priority[worker] ==
							config->priority[curr_workers[index]])
						{
							double worker_idle_time = sc_w->current_idle_time[worker];
							double curr_worker_idle_time = sc_w->current_idle_time[curr_workers[index]];
							if(worker_idle_time > curr_worker_idle_time)
								curr_workers[index] = worker;
						}
					}
				}
			}
		}

		if(curr_workers[index] < 0)
		{
			*nworkers = index;
			break;
		}
	}

	if(workers->init_cursor)
		workers->deinit_cursor(workers);

	return curr_workers;
}

/* get the number of workers in the context that are allowed to be moved (that are not fixed) */
unsigned _get_potential_nworkers(struct policy_config *config, unsigned sched_ctx, enum starpu_archtype arch)
{
	struct worker_collection *workers = starpu_get_worker_collection_of_sched_ctx(sched_ctx);

	unsigned potential_workers = 0;
	int worker;

	if(workers->init_cursor)
		workers->init_cursor(workers);
	while(workers->has_next(workers))
	{
		worker = workers->get_next(workers);
		enum starpu_archtype curr_arch = starpu_worker_get_type(worker);
                if(arch == STARPU_ANY_WORKER || curr_arch == arch)
                {
			if(!config->fixed_workers[worker])
				potential_workers++;
		}
	}
	if(workers->init_cursor)
		workers->deinit_cursor(workers);

	return potential_workers;
}

/* compute the number of workers that should be moved depending:
   - on the min/max number of workers in a context imposed by the user,
   - on the resource granularity imposed by the user for the resizing process*/
int _get_nworkers_to_move(unsigned req_sched_ctx)
{
       	struct policy_config *config = sched_ctx_hypervisor_get_config(req_sched_ctx);
	unsigned nworkers = starpu_get_nworkers_of_sched_ctx(req_sched_ctx);
	unsigned nworkers_to_move = 0;

	unsigned potential_moving_workers = _get_potential_nworkers(config, req_sched_ctx, STARPU_ANY_WORKER);
	if(potential_moving_workers > 0)
	{
		if(potential_moving_workers <= config->min_nworkers)
			/* if we have to give more than min better give it all */
			/* => empty ctx will block until having the required workers */
			nworkers_to_move = potential_moving_workers;
		else if(potential_moving_workers > config->max_nworkers)
		{
			if((potential_moving_workers - config->granularity) > config->max_nworkers)
//				nworkers_to_move = config->granularity;
				nworkers_to_move = potential_moving_workers;
			else
				nworkers_to_move = potential_moving_workers - config->max_nworkers;

		}
		else if(potential_moving_workers > config->granularity)
		{
			if((nworkers - config->granularity) > config->min_nworkers)
				nworkers_to_move = config->granularity;
			else
				nworkers_to_move = potential_moving_workers - config->min_nworkers;
		}
		else
		{
			int nfixed_workers = nworkers - potential_moving_workers;
			if(nfixed_workers >= config->min_nworkers)
				nworkers_to_move = potential_moving_workers;
			else
				nworkers_to_move = potential_moving_workers - (config->min_nworkers - nfixed_workers);
		}

		if((nworkers - nworkers_to_move) > config->max_nworkers)
			nworkers_to_move = nworkers - config->max_nworkers;
	}
	return nworkers_to_move;
}

unsigned _resize(unsigned sender_sched_ctx, unsigned receiver_sched_ctx, unsigned force_resize, unsigned now)
{
	int ret = 1;
	if(force_resize)
		pthread_mutex_lock(&act_hypervisor_mutex);
	else
		ret = pthread_mutex_trylock(&act_hypervisor_mutex);
	if(ret != EBUSY)
	{
		int nworkers_to_move = _get_nworkers_to_move(sender_sched_ctx);
		if(nworkers_to_move > 0)
		{
			unsigned poor_sched_ctx = STARPU_NMAX_SCHED_CTXS;
			if(receiver_sched_ctx == STARPU_NMAX_SCHED_CTXS)
			{
				poor_sched_ctx = _find_poor_sched_ctx(sender_sched_ctx, nworkers_to_move);
			}
			else
			{
				poor_sched_ctx = receiver_sched_ctx;
				struct policy_config *config = sched_ctx_hypervisor_get_config(poor_sched_ctx);
				unsigned nworkers = starpu_get_nworkers_of_sched_ctx(poor_sched_ctx);
				unsigned nshared_workers = starpu_get_nshared_workers(sender_sched_ctx, poor_sched_ctx);
				if((nworkers+nworkers_to_move-nshared_workers) > config->max_nworkers)
					nworkers_to_move = nworkers > config->max_nworkers ? 0 : (config->max_nworkers - nworkers+nshared_workers);
				if(nworkers_to_move == 0) poor_sched_ctx = STARPU_NMAX_SCHED_CTXS;
			}
			if(poor_sched_ctx != STARPU_NMAX_SCHED_CTXS)
			{
				int *workers_to_move = _get_first_workers(sender_sched_ctx, &nworkers_to_move, STARPU_ANY_WORKER);
				sched_ctx_hypervisor_move_workers(sender_sched_ctx, poor_sched_ctx, workers_to_move, nworkers_to_move, now);

				struct policy_config *new_config = sched_ctx_hypervisor_get_config(poor_sched_ctx);
				int i;
				for(i = 0; i < nworkers_to_move; i++)
					new_config->max_idle[workers_to_move[i]] = new_config->max_idle[workers_to_move[i]] !=MAX_IDLE_TIME ? new_config->max_idle[workers_to_move[i]] :  new_config->new_workers_max_idle;

				free(workers_to_move);
			}
		}
		pthread_mutex_unlock(&act_hypervisor_mutex);
		return 1;
	}
	return 0;

}


unsigned _resize_to_unknown_receiver(unsigned sender_sched_ctx, unsigned now)
{
	return _resize(sender_sched_ctx, STARPU_NMAX_SCHED_CTXS, 0, now);
}

static double _get_elapsed_flops(struct sched_ctx_wrapper* sc_w, int *npus, enum starpu_archtype req_arch)
{
	double ret_val = 0.0;
	struct worker_collection *workers = starpu_get_worker_collection_of_sched_ctx(sc_w->sched_ctx);
        int worker;

	if(workers->init_cursor)
                workers->init_cursor(workers);

        while(workers->has_next(workers))
	{
                worker = workers->get_next(workers);
                enum starpu_archtype arch = starpu_worker_get_type(worker);
                if(arch == req_arch)
                {
			ret_val += sc_w->elapsed_flops[worker];
			(*npus)++;
                }
        }

	if(workers->init_cursor)
		workers->deinit_cursor(workers);

	return ret_val;
}

double _get_ctx_velocity(struct sched_ctx_wrapper* sc_w)
{
        double elapsed_flops = sched_ctx_hypervisor_get_elapsed_flops_per_sched_ctx(sc_w);
	double total_elapsed_flops = sched_ctx_hypervisor_get_total_elapsed_flops_per_sched_ctx(sc_w);
	double prc = elapsed_flops/sc_w->total_flops;
	unsigned nworkers = starpu_get_nworkers_of_sched_ctx(sc_w->sched_ctx);
	double redim_sample = elapsed_flops == total_elapsed_flops ? HYPERVISOR_START_REDIM_SAMPLE*nworkers : HYPERVISOR_REDIM_SAMPLE*nworkers;
	if(prc >= redim_sample)
        {
                double curr_time = starpu_timing_now();
                double elapsed_time = curr_time - sc_w->start_time;
                return elapsed_flops/elapsed_time;
        }
	return 0.0;
}

/* compute an average value of the cpu velocity */
double _get_velocity_per_worker_type(struct sched_ctx_wrapper* sc_w, enum starpu_archtype arch)
{
        int npus = 0;
        double elapsed_flops = _get_elapsed_flops(sc_w, &npus, arch);

        if( elapsed_flops != 0.0)
        {
                double curr_time = starpu_timing_now();
                double elapsed_time = curr_time - sc_w->start_time;
                return (elapsed_flops/elapsed_time) / npus;
        }

        return -1.0;
}


/* check if there is a big velocity gap between the contexts */
int _velocity_gap_btw_ctxs()
{
	int *sched_ctxs = sched_ctx_hypervisor_get_sched_ctxs();
	int nsched_ctxs = sched_ctx_hypervisor_get_nsched_ctxs();
	int i = 0, j = 0;
	struct sched_ctx_wrapper* sc_w;
	struct sched_ctx_wrapper* other_sc_w;

	for(i = 0; i < nsched_ctxs; i++)
	{
		sc_w = sched_ctx_hypervisor_get_wrapper(sched_ctxs[i]);
		double ctx_v = _get_ctx_velocity(sc_w);
		if(ctx_v != 0.0)
		{
			for(j = 0; j < nsched_ctxs; j++)
			{
				if(sched_ctxs[i] != sched_ctxs[j])
				{
					other_sc_w = sched_ctx_hypervisor_get_wrapper(sched_ctxs[j]);
					double other_ctx_v = _get_ctx_velocity(other_sc_w);
					if(other_ctx_v != 0.0)
					{
						double gap = ctx_v < other_ctx_v ? other_ctx_v / ctx_v : ctx_v / other_ctx_v ;
						if(gap > 1.5)
							return 1;
					}
					else
						return 1;
				}
			}
		}

	}
	return 0;
}


void _get_total_nw(int *workers, int nworkers, int ntypes_of_workers, int total_nw[ntypes_of_workers])
{
	int current_nworkers = workers == NULL ? starpu_worker_get_count() : nworkers;
	int w;
	for(w = 0; w < ntypes_of_workers; w++)
		total_nw[w] = 0;

	for(w = 0; w < current_nworkers; w++)
	{
		enum starpu_archtype arch = workers == NULL ? starpu_worker_get_type(w) :
			starpu_worker_get_type(workers[w]);
		if(arch == STARPU_CPU_WORKER)
			total_nw[1]++;
		else
			total_nw[0]++;
	}
}
