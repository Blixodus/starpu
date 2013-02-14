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

#include <math.h>
#include "lp_tools.h"
#include <starpu_config.h>

#ifdef STARPU_HAVE_GLPK_H

double _lp_compute_nworkers_per_ctx(int ns, int nw, double v[ns][nw], double flops[ns], double res[ns][nw], int  total_nw[nw])
{
	int s, w;
	glp_prob *lp;

	int ne =
		(ns*nw+1)*(ns+nw)
		+ 1; /* glp dumbness */
	int n = 1;
	int ia[ne], ja[ne];
	double ar[ne];

	lp = glp_create_prob();

	glp_set_prob_name(lp, "sample");
	glp_set_obj_dir(lp, GLP_MAX);
        glp_set_obj_name(lp, "max speed");

	/* we add nw*ns columns one for each type of worker in each context
	   and another column corresponding to the 1/tmax bound (bc 1/tmax is a variable too)*/
	glp_add_cols(lp, nw*ns+1);

	for(s = 0; s < ns; s++)
	{
		for(w = 0; w < nw; w++)
		{
			char name[32];
			snprintf(name, sizeof(name), "worker%dctx%d", w, s);
			glp_set_col_name(lp, n, name);
			glp_set_col_bnds(lp, n, GLP_LO, 0.3, 0.0);
			n++;
		}
	}

	/*1/tmax should belong to the interval [0.0;1.0]*/
	glp_set_col_name(lp, n, "vmax");
	glp_set_col_bnds(lp, n, GLP_DB, 0.0, 1.0);
	/* Z = 1/tmax -> 1/tmax structural variable, nCPUs & nGPUs in ctx are auxiliar variables */
	glp_set_obj_coef(lp, n, 1.0);

	n = 1;
	/* one row corresponds to one ctx*/
	glp_add_rows(lp, ns);

	for(s = 0; s < ns; s++)
	{
		char name[32];
		snprintf(name, sizeof(name), "ctx%d", s);
		glp_set_row_name(lp, s+1, name);
		glp_set_row_bnds(lp, s+1, GLP_LO, 0., 0.);

		for(w = 0; w < nw; w++)
		{
			int s2;
			for(s2 = 0; s2 < ns; s2++)
			{
				if(s2 == s)
				{
					ia[n] = s+1;
					ja[n] = w + nw*s2 + 1;
					ar[n] = v[s][w];
//					printf("ia[%d]=%d ja[%d]=%d ar[%d]=%lf\n", n, ia[n], n, ja[n], n, ar[n]);
				}
				else
				{
					ia[n] = s+1;
					ja[n] = w + nw*s2 + 1;
					ar[n] = 0.0;
//					printf("ia[%d]=%d ja[%d]=%d ar[%d]=%lf\n", n, ia[n], n, ja[n], n, ar[n]);
				}
				n++;
			}
		}
		/* 1/tmax */
		ia[n] = s+1;
		ja[n] = ns*nw+1;
		ar[n] = (-1) * flops[s];
//		printf("ia[%d]=%d ja[%d]=%d ar[%d]=%lf\n", n, ia[n], n, ja[n], n, ar[n]);
		n++;
	}

	/*we add another linear constraint : sum(all cpus) = 9 and sum(all gpus) = 3 */
	glp_add_rows(lp, nw);

	for(w = 0; w < nw; w++)
	{
		char name[32];
		snprintf(name, sizeof(name), "w%d", w);
		glp_set_row_name(lp, ns+w+1, name);
		for(s = 0; s < ns; s++)
		{
			int w2;
			for(w2 = 0; w2 < nw; w2++)
			{
				if(w2 == w)
				{
					ia[n] = ns+w+1;
					ja[n] = w2+s*nw + 1;
					ar[n] = 1.0;
//					printf("ia[%d]=%d ja[%d]=%d ar[%d]=%lf\n", n, ia[n], n, ja[n], n, ar[n]);
				}
				else
				{
					ia[n] = ns+w+1;
					ja[n] = w2+s*nw + 1;
					ar[n] = 0.0;
//					printf("ia[%d]=%d ja[%d]=%d ar[%d]=%lf\n", n, ia[n], n, ja[n], n, ar[n]);
				}
				n++;
			}
		}
		/* 1/tmax */
		ia[n] = ns+w+1;
		ja[n] = ns*nw+1;
		ar[n] = 0.0;
//		printf("ia[%d]=%d ja[%d]=%d ar[%d]=%lf\n", n, ia[n], n, ja[n], n, ar[n]);
		n++;

		/*sum(all gpus) = 3*/
		if(w == 0)
			glp_set_row_bnds(lp, ns+w+1, GLP_FX, total_nw[0], total_nw[0]);

		/*sum(all cpus) = 9*/
		if(w == 1)
			glp_set_row_bnds(lp, ns+w+1, GLP_FX, total_nw[1], total_nw[1]);
	}

	STARPU_ASSERT(n == ne);

	glp_load_matrix(lp, ne-1, ia, ja, ar);

	glp_smcp parm;
	glp_init_smcp(&parm);
	parm.msg_lev = GLP_MSG_OFF;
	glp_simplex(lp, &parm);

	double vmax = glp_get_obj_val(lp);

	n = 1;
	for(s = 0; s < ns; s++)
	{
		for(w = 0; w < nw; w++)
		{
			res[s][w] = glp_get_col_prim(lp, n);
			n++;
		}
	}

	glp_delete_prob(lp);
	return vmax;
}

#endif //STARPU_HAVE_GLPK_H

double _lp_get_nworkers_per_ctx(int nsched_ctxs, int ntypes_of_workers, double res[nsched_ctxs][ntypes_of_workers], int total_nw[ntypes_of_workers])
{
	int *sched_ctxs = sched_ctx_hypervisor_get_sched_ctxs();
#ifdef STARPU_HAVE_GLPK_H
	double v[nsched_ctxs][ntypes_of_workers];
	double flops[nsched_ctxs];

	int i = 0;
	struct sched_ctx_hypervisor_wrapper* sc_w;
	for(i = 0; i < nsched_ctxs; i++)
	{
		sc_w = sched_ctx_hypervisor_get_wrapper(sched_ctxs[i]);
		v[i][0] = 200.0;//_get_velocity_per_worker_type(sc_w, STARPU_CUDA_WORKER);
		v[i][1] = 20.0;//_get_velocity_per_worker_type(sc_w, STARPU_CPU_WORKER);
		flops[i] = sc_w->remaining_flops/1000000000; //sc_w->total_flops/1000000000; /* in gflops*/
//			printf("%d: flops %lf\n", sched_ctxs[i], flops[i]);
	}

	return 1/_lp_compute_nworkers_per_ctx(nsched_ctxs, ntypes_of_workers, v, flops, res, total_nw);
#else
	return 0.0;
#endif
}

double _lp_get_tmax(int nw, int *workers)
{
	int ntypes_of_workers = 2;
	int total_nw[ntypes_of_workers];
	_get_total_nw(workers, nw, 2, total_nw);

	int nsched_ctxs = sched_ctx_hypervisor_get_nsched_ctxs();

	double res[nsched_ctxs][ntypes_of_workers];
	return _lp_get_nworkers_per_ctx(nsched_ctxs, ntypes_of_workers, res, total_nw) * 1000;
}

void _lp_round_double_to_int(int ns, int nw, double res[ns][nw], int res_rounded[ns][nw])
{
	int s, w;
	double left_res[nw];
	for(w = 0; w < nw; w++)
		left_res[nw] = 0.0;
	for(s = 0; s < ns; s++)
	{
		for(w = 0; w < nw; w++)
		{
			int x = floor(res[s][w]);
			double x_double = (double)x;
			double diff = res[s][w] - x_double;

			if(diff != 0.0)
			{
				if(diff > 0.5)
				{
					if(left_res[w] != 0.0)
					{
						if((diff + left_res[w]) > 0.5)
						{
							res_rounded[s][w] = x + 1;
							left_res[w] = (-1.0) * (x_double + 1.0 - (res[s][w] + left_res[w]));
						}
						else
						{
							res_rounded[s][w] = x;
							left_res[w] = (-1.0) * (diff + left_res[w]);
						}
					}
					else
					{
						res_rounded[s][w] = x + 1;
						left_res[w] = (-1.0) * (x_double + 1.0 - res[s][w]);
					}

				}
				else
				{
					if((diff + left_res[w]) > 0.5)
					{
						res_rounded[s][w] = x + 1;
						left_res[w] = (-1.0) * (x_double + 1.0 - (res[s][w] + left_res[w]));
					}
					else
					{
						res_rounded[s][w] = x;
						left_res[w] = diff;
					}
				}
			}
		}
	}
}

void _lp_redistribute_resources_in_ctxs(int ns, int nw, int res_rounded[ns][nw], double res[ns][nw])
{
	int *sched_ctxs = sched_ctx_hypervisor_get_sched_ctxs();
	int s, s2, w;
	for(s = 0; s < ns; s++)
	{
		int workers_move[STARPU_NMAXWORKERS];
		int nw_move = 0;
		
		int workers_add[STARPU_NMAXWORKERS];
		int nw_add = 0;

		for(w = 0; w < nw; w++)
		{
			enum starpu_archtype arch;
			if(w == 0) arch = STARPU_CUDA_WORKER;
			if(w == 1) arch = STARPU_CPU_WORKER;

			if(w == 1)
			{
				int nworkers_ctx = sched_ctx_hypervisor_get_nworkers_ctx(sched_ctxs[s], arch);
				if(nworkers_ctx > res_rounded[s][w])
				{
					int nworkers_to_move = nworkers_ctx - res_rounded[s][w];
					int *workers_to_move = _get_first_workers(sched_ctxs[s], &nworkers_to_move, arch);
					int i;
					for(i = 0; i < nworkers_to_move; i++)
						workers_move[nw_move++] = workers_to_move[i];
					free(workers_to_move);
				}
			}
			else
			{
				double nworkers_ctx = sched_ctx_hypervisor_get_nworkers_ctx(sched_ctxs[s], arch) * 1.0;
				if(nworkers_ctx > res[s][w])
				{
					double nworkers_to_move = nworkers_ctx - res[s][w];
					int x = floor(nworkers_to_move);
					double x_double = (double)x;
					double diff = nworkers_to_move - x_double;
					if(diff == 0.0)
					{
						int *workers_to_move = _get_first_workers(sched_ctxs[s], &x, arch);
						if(x > 0)
						{
							int i;
							for(i = 0; i < x; i++)
								workers_move[nw_move++] = workers_to_move[i];

						}
						free(workers_to_move);
					}
					else
					{
						x+=1;
						int *workers_to_move = _get_first_workers(sched_ctxs[s], &x, arch);
						if(x > 0)
						{
							int i;
							for(i = 0; i < x-1; i++)
								workers_move[nw_move++] = workers_to_move[i];

							if(diff > 0.8)
								workers_move[nw_move++] = workers_to_move[x-1];
							else
								if(diff > 0.3)
									workers_add[nw_add++] = workers_to_move[x-1];

						}
						free(workers_to_move);
					}
				}
			}
		}

		for(s2 = 0; s2 < ns; s2++)
		{
			if(sched_ctxs[s2] != sched_ctxs[s])
			{
				double nworkers_ctx2 = sched_ctx_hypervisor_get_nworkers_ctx(sched_ctxs[s2], STARPU_ANY_WORKER) * 1.0;
				int total_res = 0;
				for(w = 0; w < nw; w++)
					total_res += res[s2][w];
//				if(( total_res - nworkers_ctx2) >= 0.0 && nw_move > 0)
				if(nw_move > 0)
				{
					sched_ctx_hypervisor_move_workers(sched_ctxs[s], sched_ctxs[s2], workers_move, nw_move, 0);
					nw_move = 0;
//					break;
				}
//				if((total_res - nworkers_ctx2) >= 0.0 &&  (total_res - nworkers_ctx2) <= (double)nw_add && nw_add > 0)
				if(nw_add > 0)
				{
					sched_ctx_hypervisor_add_workers_to_sched_ctx(workers_add, nw_add, sched_ctxs[s2]);
					nw_add = 0;
//					break;
				}
				
			}
		}
		if(nw_move > 0)
			sched_ctx_hypervisor_remove_workers_from_sched_ctx(workers_move, nw_move, sched_ctxs[s], 0);
	}
}

void _lp_distribute_resources_in_ctxs(int* sched_ctxs, int ns, int nw, int res_rounded[ns][nw], double res[ns][nw], int *workers, int nworkers)
{
	int current_nworkers = workers == NULL ? starpu_worker_get_count() : nworkers;
	int *current_sched_ctxs = sched_ctxs == NULL ? sched_ctx_hypervisor_get_sched_ctxs() : sched_ctxs;

	int s, w;
	for(s = 0; s < ns; s++)
	{
		for(w = 0; w < nw; w++)
		{
			enum starpu_archtype arch;
			if(w == 0) arch = STARPU_CUDA_WORKER;
			if(w == 1) arch = STARPU_CPU_WORKER;

			if(w == 1)
			{
				int nworkers_to_add = res_rounded[s][w];
				int *workers_to_add = _get_first_workers_in_list(workers, current_nworkers, &nworkers_to_add, arch);

				if(nworkers_to_add > 0)
				{
					sched_ctx_hypervisor_add_workers_to_sched_ctx(workers_to_add, nworkers_to_add, current_sched_ctxs[s]);
					sched_ctx_hypervisor_start_resize(current_sched_ctxs[s]);
					struct sched_ctx_hypervisor_policy_config *new_config = sched_ctx_hypervisor_get_config(current_sched_ctxs[s]);
					int i;
					for(i = 0; i < nworkers_to_add; i++)
						new_config->max_idle[workers_to_add[i]] = new_config->max_idle[workers_to_add[i]] != MAX_IDLE_TIME ? new_config->max_idle[workers_to_add[i]] :  new_config->new_workers_max_idle;
				}
				free(workers_to_add);
			}
			else
			{
				double nworkers_to_add = res[s][w];
				int x = floor(nworkers_to_add);
				double x_double = (double)x;
				double diff = nworkers_to_add - x_double;
				if(diff == 0.0)
				{
					int *workers_to_add = _get_first_workers_in_list(workers, current_nworkers, &x, arch);
					if(x > 0)
					{
						sched_ctx_hypervisor_add_workers_to_sched_ctx(workers_to_add, x, current_sched_ctxs[s]);
						sched_ctx_hypervisor_start_resize(current_sched_ctxs[s]);
					}
					free(workers_to_add);
				}
				else
				{
					x+=1;
					int *workers_to_add = _get_first_workers_in_list(workers, current_nworkers, &x, arch);
					if(x > 0)
					{
						if(diff >= 0.3)
							sched_ctx_hypervisor_add_workers_to_sched_ctx(workers_to_add, x, current_sched_ctxs[s]);
						else
							sched_ctx_hypervisor_add_workers_to_sched_ctx(workers_to_add, x-1, current_sched_ctxs[s]);
						sched_ctx_hypervisor_start_resize(current_sched_ctxs[s]);
					}
					free(workers_to_add);
				}
			}

		}
		sched_ctx_hypervisor_stop_resize(current_sched_ctxs[s]);
	}
}
