/*
 * StarPU
 * Copyright (C) Université Bordeaux 1, CNRS 2008-2011 (see AUTHORS file)
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

#include <math.h>
#include <starpu.h>
#include <starpu_profiling.h>
#include <profiling/profiling.h>
#include <common/utils.h>
#include <core/debug.h>
#include <drivers/driver_common/driver_common.h>

void _starpu_driver_update_job_feedback(starpu_job_t j, struct starpu_worker_s *worker_args,
					struct starpu_task_profiling_info *profiling_info,
					unsigned calibrate_model, enum starpu_perf_archtype perf_arch,
					struct timespec *codelet_start, struct timespec *codelet_end)
{
	struct timespec measured_ts;
	double measured;
	int workerid = worker_args->workerid;

	if (profiling_info || calibrate_model)
	{
		starpu_timespec_sub(codelet_end, codelet_start, &measured_ts);
		measured = starpu_timing_timespec_to_us(&measured_ts);

		if (starpu_profiling_status_get())
		{
			if (profiling_info)
			{
				memcpy(&profiling_info->start_time, codelet_start, sizeof(struct timespec));
				memcpy(&profiling_info->end_time, codelet_end, sizeof(struct timespec));

				profiling_info->workerid = workerid;
				
				_starpu_worker_update_profiling_info_executing(workerid, &measured_ts, 1,
					profiling_info->used_cycles,
					profiling_info->stall_cycles,
					profiling_info->power_consumed);
			}
		} else {
			_starpu_worker_update_profiling_info_executing(workerid, 0, 1, 0, 0, 0);
		}

		if (calibrate_model) {
			if (profiling_info && profiling_info->power_consumed) {
				/* TODO: update power model history */
			}
			_starpu_update_perfmodel_history(j, perf_arch, worker_args->devid, measured);
		}
	} else {
		_starpu_worker_update_profiling_info_executing(workerid, 0, 1, 0, 0, 0);
	}
}

/* Workers may block when there is no work to do at all. We assume that the
 * mutex is hold when that function is called. */
void _starpu_block_worker(int workerid, pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	struct timespec start_time, end_time;

	STARPU_TRACE_WORKER_SLEEP_START
	_starpu_worker_set_status(workerid, STATUS_SLEEPING);

	starpu_clock_gettime(&start_time);
	_starpu_worker_register_sleeping_start_date(workerid, &start_time);

	PTHREAD_COND_WAIT(cond, mutex);

	_starpu_worker_set_status(workerid, STATUS_UNKNOWN);
	STARPU_TRACE_WORKER_SLEEP_END
	starpu_clock_gettime(&end_time);

	int profiling = starpu_profiling_status_get();
	if (profiling)
	{
		struct timespec sleeping_time;
		starpu_timespec_sub(&end_time, &start_time, &sleeping_time);
		_starpu_worker_update_profiling_info_sleeping(workerid, &start_time, &end_time);
	}
}
