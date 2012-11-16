/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2009-2012  Université de Bordeaux 1
 * Copyright (C) 2010, 2011, 2012  Centre National de la Recherche Scientifique
 * Copyright (C) 2011  Télécom-SudParis
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

#ifndef __PERFMODEL_H__
#define __PERFMODEL_H__

#include <common/config.h>
#include <starpu.h>
#include <starpu_perfmodel.h>
#include <core/task_bundle.h>
#include <pthread.h>
#include <stdio.h>

struct _starpu_perfmodel_list
{
	struct _starpu_perfmodel_list *next;
	struct starpu_perfmodel *model;
};

struct starpu_buffer_descr;
struct _starpu_job;
enum starpu_perf_archtype;

void _starpu_get_perf_model_dir(char *path, size_t maxlen);
void _starpu_get_perf_model_dir_codelets(char *path, size_t maxlen);
void _starpu_get_perf_model_dir_bus(char *path, size_t maxlen);
void _starpu_get_perf_model_dir_debug(char *path, size_t maxlen);

double _starpu_history_based_job_expected_perf(struct starpu_perfmodel *model, enum starpu_perf_archtype arch, struct _starpu_job *j, unsigned nimpl);
int _starpu_register_model(struct starpu_perfmodel *model);
void _starpu_load_per_arch_based_model(struct starpu_perfmodel *model);
void _starpu_load_common_based_model(struct starpu_perfmodel *model);
void _starpu_load_history_based_model(struct starpu_perfmodel *model, unsigned scan_history);
void _starpu_load_perfmodel(struct starpu_perfmodel *model);
void _starpu_initialize_registered_performance_models(void);
void _starpu_deinitialize_registered_performance_models(void);

double _starpu_regression_based_job_expected_perf(struct starpu_perfmodel *model,
					enum starpu_perf_archtype arch, struct _starpu_job *j, unsigned nimpl);
double _starpu_non_linear_regression_based_job_expected_perf(struct starpu_perfmodel *model,
					enum starpu_perf_archtype arch, struct _starpu_job *j, unsigned nimpl);
void _starpu_update_perfmodel_history(struct _starpu_job *j, struct starpu_perfmodel *model, enum starpu_perf_archtype arch,
				unsigned cpuid, double measured, unsigned nimpl);

void _starpu_create_sampling_directory_if_needed(void);

void _starpu_load_bus_performance_files(void);
double _starpu_transfer_bandwidth(unsigned src_node, unsigned dst_node);
double _starpu_transfer_latency(unsigned src_node, unsigned dst_node);
double _starpu_predict_transfer_time(unsigned src_node, unsigned dst_node, size_t size);


void _starpu_set_calibrate_flag(unsigned val);
unsigned _starpu_get_calibrate_flag(void);

#if defined(STARPU_USE_CUDA)
int *_starpu_get_cuda_affinity_vector(unsigned gpuid);
#endif
#if defined(STARPU_USE_OPENCL)
int *_starpu_get_opencl_affinity_vector(unsigned gpuid);
#endif

#endif // __PERFMODEL_H__
