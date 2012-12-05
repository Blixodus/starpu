/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2009-2012  Université de Bordeaux 1
 * Copyright (C) 2010, 2011, 2012  Centre National de la Recherche Scientifique
 * Copyright (C) 2010, 2011  Institut National de Recherche en Informatique et Automatique
 * Copyright (C) 2011  Télécom-SudParis
 * Copyright (C) 2011-2012  INRIA
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

#include <stdlib.h>
#include <stdio.h>
#include <common/config.h>
#include <common/utils.h>
#include <core/progress_hook.h>
#include <core/workers.h>
#include <core/debug.h>
#include <core/task.h>
#include <profiling/profiling.h>
#include <starpu_task_list.h>

#include <drivers/cpu/driver_cpu.h>
#include <drivers/cuda/driver_cuda.h>
#include <drivers/opencl/driver_opencl.h>

#ifdef STARPU_SIMGRID
#include <msg/msg.h>
#endif

#ifdef __MINGW32__
#include <windows.h>
#endif

/* acquire/release semantic for concurrent initialization/de-initialization */
static _starpu_pthread_mutex_t init_mutex = _STARPU_PTHREAD_MUTEX_INITIALIZER;
static _starpu_pthread_cond_t init_cond = _STARPU_PTHREAD_COND_INITIALIZER;
static int init_count = 0;
static enum { UNINITIALIZED, CHANGING, INITIALIZED } initialized = UNINITIALIZED;

static _starpu_pthread_key_t worker_key;

static struct _starpu_machine_config config;

int _starpu_is_initialized(void)
{
	return initialized == INITIALIZED;
}

struct _starpu_machine_config *_starpu_get_machine_config(void)
{
	return &config;
}

/* Makes sure that at least one of the workers of type <arch> can execute
 * <task>, for at least one of its implementations. */
static uint32_t _starpu_worker_exists_and_can_execute(struct starpu_task *task,
						      enum starpu_archtype arch)
{
	int i;
	int nworkers = starpu_worker_get_count();
	for (i = 0; i < nworkers; i++)
	{
		if (starpu_worker_get_type(i) != arch)
			continue;

		unsigned impl;
		for (impl = 0; impl < STARPU_MAXIMPLEMENTATIONS; impl++)
		{
			/* We could call task->cl->can_execute(i, task, impl) 
			   here, it would definitely work. It is probably
			   cheaper to check whether it is necessary in order to
			   avoid a useless function call, though. */
			unsigned test_implementation = 0;
			switch (arch)
			{
			case STARPU_CPU_WORKER:
				if (task->cl->cpu_funcs[i] != NULL)
					test_implementation = 1;
				break;
			case STARPU_CUDA_WORKER:
				if (task->cl->cuda_funcs[i] != NULL)
					test_implementation = 1;
				break;
			case STARPU_OPENCL_WORKER:
				if (task->cl->opencl_funcs[i] != NULL)
					test_implementation = 1;
				break;
			case STARPU_GORDON_WORKER:
				if (task->cl->gordon_funcs[i] != 0)
					test_implementation = 1;
				break;
			default:
				STARPU_ABORT();
			}

			if (!test_implementation)
				break;

			if (task->cl->can_execute(i, task, impl))
				return 1;
		}
	}

	return 0;
}

/* in case a task is submitted, we may check whether there exists a worker
   that may execute the task or not */
uint32_t _starpu_worker_exists(struct starpu_task *task)
{
	if (!(task->cl->where & config.worker_mask))
		return 0;

	if (!task->cl->can_execute)
		return 1;

#ifdef STARPU_USE_CPU
	if ((task->cl->where & STARPU_CPU) &&
	    _starpu_worker_exists_and_can_execute(task, STARPU_CPU_WORKER))
		return 1;
#endif
#ifdef STARPU_USE_CUDA
	if ((task->cl->where & STARPU_CUDA) &&
	    _starpu_worker_exists_and_can_execute(task, STARPU_CUDA_WORKER))
		return 1;
#endif
#ifdef STARPU_USE_OPENCL
	if ((task->cl->where & STARPU_OPENCL) &&
	    _starpu_worker_exists_and_can_execute(task, STARPU_OPENCL_WORKER))
		return 1;
#endif
	return 0;
}

uint32_t _starpu_can_submit_cuda_task(void)
{
	return (STARPU_CUDA & config.worker_mask);
}

uint32_t _starpu_can_submit_cpu_task(void)
{
	return (STARPU_CPU & config.worker_mask);
}

uint32_t _starpu_can_submit_opencl_task(void)
{
	return (STARPU_OPENCL & config.worker_mask);
}

static int _starpu_can_use_nth_implementation(enum starpu_archtype arch, struct starpu_codelet *cl, unsigned nimpl)
{
	switch(arch)
	{
	case STARPU_CPU_WORKER:
	{
		starpu_cpu_func_t func = _starpu_task_get_cpu_nth_implementation(cl, nimpl);
		return func != NULL;
	}
	case STARPU_CUDA_WORKER:
	{
		starpu_cuda_func_t func = _starpu_task_get_cuda_nth_implementation(cl, nimpl);
		return func != NULL;
	}
	case STARPU_OPENCL_WORKER:
	{
		starpu_opencl_func_t func = _starpu_task_get_opencl_nth_implementation(cl, nimpl);
		return func != NULL;
	}
	case STARPU_GORDON_WORKER:
	{
		starpu_gordon_func_t func = _starpu_task_get_gordon_nth_implementation(cl, nimpl);
		return func != 0;
	}
	default:
		STARPU_ASSERT_MSG(0, "Unknown arch type %d", arch);
	}
	return 0;
}


int starpu_worker_can_execute_task(unsigned workerid, struct starpu_task *task, unsigned nimpl)
{
	/* TODO: check that the task operand sizes will fit on that device */
	return (task->cl->where & config.workers[workerid].worker_mask) &&
		_starpu_can_use_nth_implementation(config.workers[workerid].arch, task->cl, nimpl) &&
		(!task->cl->can_execute || task->cl->can_execute(workerid, task, nimpl));
}



int starpu_combined_worker_can_execute_task(unsigned workerid, struct starpu_task *task, unsigned nimpl)
{
	/* TODO: check that the task operand sizes will fit on that device */
	/* TODO: call application-provided function for various cases like
	 * double support, shared memory size limit, etc. */

	struct starpu_codelet *cl = task->cl;
	unsigned nworkers = config.topology.nworkers;

	/* Is this a parallel worker ? */
	if (workerid < nworkers)
	{
		return !!((task->cl->where & config.workers[workerid].worker_mask) &&
				_starpu_can_use_nth_implementation(config.workers[workerid].arch, task->cl, nimpl));
	}
	else
	{
		if ((cl->type == STARPU_SPMD)
#ifdef STARPU_HAVE_HWLOC
				|| (cl->type == STARPU_FORKJOIN)
#endif
				)
		{
			/* TODO we should add other types of constraints */

			/* Is the worker larger than requested ? */
			int worker_size = (int)config.combined_workers[workerid - nworkers].worker_size;
			return !!((worker_size <= task->cl->max_parallelism) &&
				_starpu_can_use_nth_implementation(config.workers[workerid].arch, task->cl, nimpl));
		}
		else
		{
			/* We have a sequential task but a parallel worker */
			return 0;
		}
	}
}

/*
 * Runtime initialization methods
 */

#ifdef STARPU_USE_GORDON
static unsigned gordon_inited = 0;
static struct _starpu_worker_set gordon_worker_set;
#endif

static void _starpu_init_worker_queue(struct _starpu_worker *workerarg)
{
	_starpu_pthread_cond_t *cond = &workerarg->sched_cond;
	_starpu_pthread_mutex_t *mutex = &workerarg->sched_mutex;

	unsigned memory_node = workerarg->memory_node;

	_starpu_memory_node_register_condition(cond, mutex, memory_node);
}

/*
 * Returns 0 if the given driver is one of the drivers that must be launched by
 * the application itself, and not by StarPU, 1 otherwise.
 */
static unsigned _starpu_may_launch_driver(struct starpu_conf *conf,
					  struct starpu_driver *d)
{
	if (conf->n_not_launched_drivers == 0 ||
	    conf->not_launched_drivers == NULL)
		return 1;

	/* Is <d> in conf->not_launched_drivers ? */
	unsigned i;
	for (i = 0; i < conf->n_not_launched_drivers; i++)
	{
		if (d->type != conf->not_launched_drivers[i].type)
			continue;

		switch (d->type)
		{
		case STARPU_CPU_WORKER:
			if (d->id.cpu_id == conf->not_launched_drivers[i].id.cpu_id)
				return 0;
		case STARPU_CUDA_WORKER:
			if (d->id.cuda_id == conf->not_launched_drivers[i].id.cuda_id)
				return 0;
			break;
#ifdef STARPU_USE_OPENCL
		case STARPU_OPENCL_WORKER:
			if (d->id.opencl_id == conf->not_launched_drivers[i].id.opencl_id)
				return 0;
			break;
#endif
		default:
			STARPU_ABORT();
		}
	}

	return 1;
}

#ifdef STARPU_PERF_DEBUG
struct itimerval prof_itimer;
#endif

void _starpu_worker_init(struct _starpu_worker *worker, unsigned fut_key)
{
	(void) fut_key;
	int devid = worker->devid;
	(void) devid;

#if defined(STARPU_PERF_DEBUG) && !defined(STARPU_SIMGRID)
	setitimer(ITIMER_PROF, &prof_itimer, NULL);
#endif

#ifdef STARPU_USE_FXT
	_starpu_fxt_register_thread(worker->bindid);

	unsigned memnode = worker->memory_node;
	_STARPU_TRACE_WORKER_INIT_START(fut_key, devid, memnode);
#endif

	_starpu_bind_thread_on_cpu(worker->config, worker->bindid);

        _STARPU_DEBUG("worker %d is ready on logical cpu %d\n", devid, worker->bindid);
#ifdef STARPU_HAVE_HWLOC
	_STARPU_DEBUG("worker %d cpuset start at %d\n", devid, hwloc_bitmap_first(worker->initial_hwloc_cpu_set));
#endif

	_starpu_set_local_memory_node_key(&worker->memory_node);

	_starpu_set_local_worker_key(worker);

}

static void _starpu_launch_drivers(struct _starpu_machine_config *config)
{
	config->running = 1;
	config->submitting = 1;

	_STARPU_PTHREAD_KEY_CREATE(&worker_key, NULL);

	unsigned nworkers = config->topology.nworkers;

	/* Launch workers asynchronously (except for SPUs) */
	unsigned cpu = 0, cuda = 0;
	unsigned worker;

#if defined(STARPU_PERF_DEBUG) && !defined(STARPU_SIMGRID)
	/* Get itimer of the main thread, to set it for the worker threads */
	getitimer(ITIMER_PROF, &prof_itimer);
#endif

#ifdef HAVE_AYUDAME_H
	if (AYU_event) {
		unsigned long n = nworkers;
		AYU_event(AYU_INIT, 0, (void*) &n);
	}
#endif

	for (worker = 0; worker < nworkers; worker++)
	{
		struct _starpu_worker *workerarg = &config->workers[worker];

		workerarg->config = config;

		_starpu_barrier_counter_init(&workerarg->tasks_barrier, 0);

		_STARPU_PTHREAD_MUTEX_INIT(&workerarg->mutex, NULL);
		_STARPU_PTHREAD_COND_INIT(&workerarg->ready_cond, NULL);

		workerarg->worker_size = 1;
		workerarg->combined_workerid = workerarg->workerid;
		workerarg->current_rank = 0;
		workerarg->has_prev_init = 0;
		/* mutex + cond only for the local list */
		/* we have a single local list */
		/* afterwards there would be a mutex + cond for the list of each strategy */
		workerarg->run_by_starpu = 1;

		_STARPU_PTHREAD_MUTEX_INIT(&workerarg->sched_mutex, NULL);
		_STARPU_PTHREAD_COND_INIT(&workerarg->sched_cond, NULL);

		/* if some codelet's termination cannot be handled directly :
		 * for instance in the Gordon driver, Gordon tasks' callbacks
		 * may be executed by another thread than that of the Gordon
		 * driver so that we cannot call the push_codelet_output method
		 * directly */
		workerarg->terminated_jobs = _starpu_job_list_new();

		starpu_task_list_init(&workerarg->local_tasks);

		workerarg->status = STATUS_INITIALIZING;

		_STARPU_DEBUG("initialising worker %u\n", worker);

		_starpu_init_worker_queue(workerarg);

		struct starpu_driver driver;
		driver.type = workerarg->arch;
		switch (workerarg->arch)
		{
#ifdef STARPU_USE_CPU
			case STARPU_CPU_WORKER:
				workerarg->set = NULL;
				workerarg->worker_is_initialized = 0;
				driver.id.cpu_id = cpu;
				if (_starpu_may_launch_driver(config->conf, &driver))
				{
					_STARPU_PTHREAD_CREATE_ON(
						workerarg->name,
						&workerarg->worker_thread,
						NULL,
						_starpu_cpu_worker,
						workerarg,
						worker+1);
				}
				else
				{
					workerarg->run_by_starpu = 0;
				}
				cpu++;
				break;
#endif
#ifdef STARPU_USE_CUDA
			case STARPU_CUDA_WORKER:
				workerarg->set = NULL;
				workerarg->worker_is_initialized = 0;
				driver.id.cuda_id = cuda;
				if (_starpu_may_launch_driver(config->conf, &driver))
				{
					_STARPU_PTHREAD_CREATE_ON(
						workerarg->name,
						&workerarg->worker_thread,
						NULL,
						_starpu_cuda_worker,
						workerarg,
						worker+1);
				}
				else
				{
					workerarg->run_by_starpu = 0;
				}
				cuda++;
				break;
#endif
#ifdef STARPU_USE_OPENCL
			case STARPU_OPENCL_WORKER:
				starpu_opencl_get_device(workerarg->devid, &driver.id.opencl_id);
				if (!_starpu_may_launch_driver(config->conf, &driver))
				{
					workerarg->run_by_starpu = 0;
					break;
				}
				workerarg->set = NULL;
				workerarg->worker_is_initialized = 0;
				_STARPU_PTHREAD_CREATE_ON(
					workerarg->name,
					&workerarg->worker_thread,
					NULL,
					_starpu_opencl_worker,
					workerarg,
					worker+1);
				break;
#endif
#ifdef STARPU_USE_GORDON
			case STARPU_GORDON_WORKER:
				/* we will only launch gordon once, but it will handle
				 * the different SPU workers */
				if (!gordon_inited)
				{
					gordon_worker_set.nworkers = config->ngordon_spus;
					gordon_worker_set.workers = &config->workers[worker];

					gordon_worker_set.set_is_initialized = 0;

					_STARPU_PTHREAD_CREATE_ON(
						workerarg->name
						&gordon_worker_set.worker_thread,
						NULL,
						_starpu_gordon_worker,
						&gordon_worker_set,
						worker+1);

					_STARPU_PTHREAD_MUTEX_LOCK(&gordon_worker_set.mutex);
					while (!gordon_worker_set.set_is_initialized)
						_STARPU_PTHREAD_COND_WAIT(&gordon_worker_set.ready_cond,
									&gordon_worker_set.mutex);
					_STARPU_PTHREAD_MUTEX_UNLOCK(&gordon_worker_set.mutex);

					gordon_inited = 1;
				}

				workerarg->set = &gordon_worker_set;
				gordon_worker_set.joined = 0;
				workerarg->worker_is_running = 1;

				break;
#endif
			default:
				STARPU_ABORT();
		}
	}

	cpu  = 0;
	cuda = 0;
	for (worker = 0; worker < nworkers; worker++)
	{
		struct _starpu_worker *workerarg = &config->workers[worker];
		struct starpu_driver driver;
		driver.type = workerarg->arch;

		switch (workerarg->arch)
		{
			case STARPU_CPU_WORKER:
				driver.id.cpu_id = cpu;
				if (!_starpu_may_launch_driver(config->conf, &driver))
				{
					cpu++;
					break;
				}
				_STARPU_PTHREAD_MUTEX_LOCK(&workerarg->mutex);
				while (!workerarg->worker_is_initialized)
					_STARPU_PTHREAD_COND_WAIT(&workerarg->ready_cond, &workerarg->mutex);
				_STARPU_PTHREAD_MUTEX_UNLOCK(&workerarg->mutex);
				cpu++;
				break;
			case STARPU_CUDA_WORKER:
				driver.id.cuda_id = cuda;
				if (!_starpu_may_launch_driver(config->conf, &driver))
				{
					cuda++;
					break;
				}
				_STARPU_PTHREAD_MUTEX_LOCK(&workerarg->mutex);
				while (!workerarg->worker_is_initialized)
					_STARPU_PTHREAD_COND_WAIT(&workerarg->ready_cond, &workerarg->mutex);
				_STARPU_PTHREAD_MUTEX_UNLOCK(&workerarg->mutex);
				cuda++;
				break;
#ifdef STARPU_USE_OPENCL
			case STARPU_OPENCL_WORKER:
				starpu_opencl_get_device(workerarg->devid, &driver.id.opencl_id);
				if (!_starpu_may_launch_driver(config->conf, &driver))
					break;
				_STARPU_PTHREAD_MUTEX_LOCK(&workerarg->mutex);
				while (!workerarg->worker_is_initialized)
					_STARPU_PTHREAD_COND_WAIT(&workerarg->ready_cond, &workerarg->mutex);
				_STARPU_PTHREAD_MUTEX_UNLOCK(&workerarg->mutex);
				break;
#endif
#ifdef STARPU_USE_GORDON
			case STARPU_GORDON_WORKER:
				/* the initialization of Gordon worker is
				 * synchronous for now */
				break;
#endif
			default:
				STARPU_ABORT();
		}
	}

}

void _starpu_set_local_worker_key(struct _starpu_worker *worker)
{
	_STARPU_PTHREAD_SETSPECIFIC(worker_key, worker);
}

struct _starpu_worker *_starpu_get_local_worker_key(void)
{
	return (struct _starpu_worker *) _STARPU_PTHREAD_GETSPECIFIC(worker_key);
}

/* Initialize the starpu_conf with default values */
int starpu_conf_init(struct starpu_conf *conf)
{
	if (!conf)
		return -EINVAL;

	memset(conf, 0, sizeof(*conf));
	conf->magic = 42;
	conf->sched_policy_name = getenv("STARPU_SCHED");
	conf->sched_policy = NULL;

	/* Note that starpu_get_env_number returns -1 in case the variable is
	 * not defined */
	/* Backward compatibility: check the value of STARPU_NCPUS if
	 * STARPU_NCPU is not set. */
	conf->ncpus = starpu_get_env_number("STARPU_NCPU");
	if (conf->ncpus == -1)
		conf->ncpus = starpu_get_env_number("STARPU_NCPUS");
	conf->ncuda = starpu_get_env_number("STARPU_NCUDA");
	conf->nopencl = starpu_get_env_number("STARPU_NOPENCL");
	conf->nspus = starpu_get_env_number("STARPU_NGORDON");
	conf->calibrate = starpu_get_env_number("STARPU_CALIBRATE");
	conf->bus_calibrate = starpu_get_env_number("STARPU_BUS_CALIBRATE");

	if (conf->calibrate == -1)
	     conf->calibrate = 0;

	if (conf->bus_calibrate == -1)
	     conf->bus_calibrate = 0;

	conf->use_explicit_workers_bindid = 0; /* TODO */
	conf->use_explicit_workers_cuda_gpuid = 0; /* TODO */
	conf->use_explicit_workers_opencl_gpuid = 0; /* TODO */

	conf->single_combined_worker = starpu_get_env_number("STARPU_SINGLE_COMBINED_WORKER");
	if (conf->single_combined_worker == -1)
	     conf->single_combined_worker = 0;

#if defined(STARPU_DISABLE_ASYNCHRONOUS_COPY)
	conf->disable_asynchronous_copy = 1;
#else
	conf->disable_asynchronous_copy = starpu_get_env_number("STARPU_DISABLE_ASYNCHRONOUS_COPY");
	if (conf->disable_asynchronous_copy == -1)
		conf->disable_asynchronous_copy = 0;
#endif

#if defined(STARPU_DISABLE_ASYNCHRONOUS_CUDA_COPY)
	conf->disable_asynchronous_cuda_copy = 1;
#else
	conf->disable_asynchronous_cuda_copy = starpu_get_env_number("STARPU_DISABLE_ASYNCHRONOUS_CUDA_COPY");
	if (conf->disable_asynchronous_cuda_copy == -1)
		conf->disable_asynchronous_cuda_copy = 0;
#endif

#if defined(STARPU_DISABLE_ASYNCHRONOUS_OPENCL_COPY)
	conf->disable_asynchronous_opencl_copy = 1;
#else
	conf->disable_asynchronous_opencl_copy = starpu_get_env_number("STARPU_DISABLE_ASYNCHRONOUS_OPENCL_COPY");
	if (conf->disable_asynchronous_opencl_copy == -1)
		conf->disable_asynchronous_opencl_copy = 0;
#endif

	return 0;
}

static void _starpu_conf_set_value_against_environment(char *name, int *value)
{
	int number;
	number = starpu_get_env_number(name);
	if (number != -1)
	{
		*value = number;
	}
}

void _starpu_conf_check_environment(struct starpu_conf *conf)
{
	char *sched = getenv("STARPU_SCHED");
	if (sched)
	{
		conf->sched_policy_name = sched;
	}

	_starpu_conf_set_value_against_environment("STARPU_NCPUS", &conf->ncpus);
	_starpu_conf_set_value_against_environment("STARPU_NCPU", &conf->ncpus);
	_starpu_conf_set_value_against_environment("STARPU_NCUDA", &conf->ncuda);
	_starpu_conf_set_value_against_environment("STARPU_NOPENCL", &conf->nopencl);
	_starpu_conf_set_value_against_environment("STARPU_NGORDON", &conf->nspus);
	_starpu_conf_set_value_against_environment("STARPU_CALIBRATE", &conf->calibrate);
	_starpu_conf_set_value_against_environment("STARPU_BUS_CALIBRATE", &conf->bus_calibrate);
	_starpu_conf_set_value_against_environment("STARPU_SINGLE_COMBINED_WORKER", &conf->single_combined_worker);
	_starpu_conf_set_value_against_environment("STARPU_DISABLE_ASYNCHRONOUS_COPY", &conf->disable_asynchronous_copy);
	_starpu_conf_set_value_against_environment("STARPU_DISABLE_ASYNCHRONOUS_CUDA_COPY", &conf->disable_asynchronous_cuda_copy);
	_starpu_conf_set_value_against_environment("STARPU_DISABLE_ASYNCHRONOUS_OPENCL_COPY", &conf->disable_asynchronous_opencl_copy);
}

int starpu_init(struct starpu_conf *user_conf)
{
	int ret;

#ifdef __GNUC__
#ifndef __OPTIMIZE__
	_STARPU_DISP("Warning: StarPU was configured with --enable-debug (-O0), and is thus not optimized\n");
#endif
#endif
#if 0
#ifndef STARPU_NO_ASSERT
	_STARPU_DISP("Warning: StarPU was configured without --enable-fast\n");
#endif
#endif
#ifdef STARPU_MEMORY_STATS
	_STARPU_DISP("Warning: StarPU was configured with --enable-memory-stats, which slows down a bit\n");
#endif
#ifdef STARPU_VERBOSE
	_STARPU_DISP("Warning: StarPU was configured with --enable-verbose, which slows down a bit\n");
#endif
#ifdef STARPU_USE_FXT
	_STARPU_DISP("Warning: StarPU was configured with --with-fxt, which slows down a bit\n");
#endif
#ifdef STARPU_PERF_DEBUG
	_STARPU_DISP("Warning: StarPU was configured with --enable-perf-debug, which slows down a bit\n");
#endif
#ifdef STARPU_MODEL_DEBUG
	_STARPU_DISP("Warning: StarPU was configured with --enable-model-debug, which slows down a bit\n");
#endif
#ifdef STARPU_ENABLE_STATS
	_STARPU_DISP("Warning: StarPU was configured with --enable-stats, which slows down a bit\n");
#endif

	_STARPU_PTHREAD_MUTEX_LOCK(&init_mutex);
	while (initialized == CHANGING)
		/* Wait for the other one changing it */
		_STARPU_PTHREAD_COND_WAIT(&init_cond, &init_mutex);
	init_count++;
	if (initialized == INITIALIZED)
	{
		/* He initialized it, don't do it again, and let the others get the mutex */
		_STARPU_PTHREAD_MUTEX_UNLOCK(&init_mutex);
		return 0;
	}
	/* initialized == UNINITIALIZED */
	initialized = CHANGING;
	_STARPU_PTHREAD_MUTEX_UNLOCK(&init_mutex);

#ifdef __MINGW32__
	WSADATA wsadata;
	WSAStartup(MAKEWORD(1,0), &wsadata);
#endif

	srand(2008);

#ifdef HAVE_AYUDAME_H
#ifndef AYU_RT_STARPU
/* Dumb value for now */
#define AYU_RT_STARPU 32
#endif
	if (AYU_event) {
		enum ayu_runtime_t ayu_rt = AYU_RT_STARPU;
		AYU_event(AYU_PREINIT, 0, (void*) &ayu_rt);
	}
#endif
	_starpu_open_debug_logfile();

	_starpu_data_interface_init();

	_starpu_timing_init();

	_starpu_profiling_init();

	/* store the pointer to the user explicit configuration during the
	 * initialization */
	if (user_conf == NULL)
	{
	     struct starpu_conf *conf = malloc(sizeof(struct starpu_conf));
	     starpu_conf_init(conf);
	     config.conf = conf;
	     config.default_conf = 1;
	}
	else
	{
	     if (user_conf->magic != 42) {
		_STARPU_DISP("starpu_conf structure needs to be initialized with starpu_conf_init\n");
		return -EINVAL;
	     }
	     config.conf = user_conf;
	     config.default_conf = 0;
	}

	_starpu_load_bus_performance_files();

	_starpu_conf_check_environment(config.conf);

	_starpu_init_all_sched_ctxs(&config);
	_starpu_init_progression_hooks();

	_starpu_init_tags();

#ifdef STARPU_USE_FXT
	_starpu_start_fxt_profiling();
#endif

	ret = _starpu_build_topology(&config);
	if (ret)
	{
		_STARPU_PTHREAD_MUTEX_LOCK(&init_mutex);
		init_count--;
		initialized = UNINITIALIZED;
		/* Let somebody else try to do it */
		_STARPU_PTHREAD_COND_SIGNAL(&init_cond);
		_STARPU_PTHREAD_MUTEX_UNLOCK(&init_mutex);
		return ret;
	}

	/* We need to store the current task handled by the different
	 * threads */
	_starpu_initialize_current_task_key();


	struct _starpu_sched_ctx *sched_ctx;
	if(user_conf == NULL)
		sched_ctx = _starpu_create_sched_ctx(NULL, NULL, -1, 1, "init");
	else
		sched_ctx = _starpu_create_sched_ctx(user_conf->sched_policy_name, NULL, -1, 1, "init");

//	starpu_set_sched_ctx(&sched_ctx->id);
	_starpu_initialize_registered_performance_models();

	/* Launch "basic" workers (ie. non-combined workers) */
	_starpu_launch_drivers(&config);

	_STARPU_PTHREAD_MUTEX_LOCK(&init_mutex);
	initialized = INITIALIZED;
	/* Tell everybody that we initialized */
	_STARPU_PTHREAD_COND_BROADCAST(&init_cond);
	_STARPU_PTHREAD_MUTEX_UNLOCK(&init_mutex);

	_STARPU_DEBUG("Initialisation finished\n");
	return 0;
}

void starpu_profiling_init()
{
	_starpu_profiling_init();
}
/*
 * Handle runtime termination
 */

static void _starpu_terminate_workers(struct _starpu_machine_config *config)
{
	int status STARPU_ATTRIBUTE_UNUSED;
	unsigned workerid;

	for (workerid = 0; workerid < config->topology.nworkers; workerid++)
	{
		starpu_wake_all_blocked_workers();

		_STARPU_DEBUG("wait for worker %u\n", workerid);

		struct _starpu_worker_set *set = config->workers[workerid].set;
		struct _starpu_worker *worker = &config->workers[workerid];

		/* in case StarPU termination code is called from a callback,
 		 * we have to check if pthread_self() is the worker itself */
		if (set)
		{
			if (!set->joined)
			{
#ifdef STARPU_SIMGRID
#ifdef STARPU_DEVEL
#warning TODO: use a simgrid_join when it becomes available
#endif
				MSG_process_sleep(1);
#else
				if (!pthread_equal(pthread_self(), set->worker_thread))
				{
					status = pthread_join(set->worker_thread, NULL);
#ifdef STARPU_VERBOSE
					if (status)
					{
						_STARPU_DEBUG("pthread_join -> %d\n", status);
                                        }
#endif
				}
#endif

				set->joined = 1;
			}
		}
		else
		{
			if (!worker->run_by_starpu)
				goto out;

#ifdef STARPU_SIMGRID
			MSG_process_sleep(1);
#else
			if (!pthread_equal(pthread_self(), worker->worker_thread))
			{
				status = pthread_join(worker->worker_thread, NULL);
#ifdef STARPU_VERBOSE
				if (status)
				{
					_STARPU_DEBUG("pthread_join -> %d\n", status);
                                }
#endif
			}
#endif
		}

out:
		STARPU_ASSERT(starpu_task_list_empty(&worker->local_tasks));
		_starpu_job_list_delete(worker->terminated_jobs);
	}
}

unsigned _starpu_machine_is_running(void)
{
	/* running is just protected by a memory barrier */
	STARPU_RMB();
	return config.running;
}

unsigned _starpu_worker_can_block(unsigned memnode STARPU_ATTRIBUTE_UNUSED)
{
#ifdef STARPU_NON_BLOCKING_DRIVERS
	return 0;
#else
	unsigned can_block = 1;

	if (!_starpu_check_that_no_data_request_exists(memnode))
		can_block = 0;

	if (!_starpu_machine_is_running())
		can_block = 0;

	if (!_starpu_execute_registered_progression_hooks())
		can_block = 0;

	return can_block;
#endif
}

static void _starpu_kill_all_workers(struct _starpu_machine_config *config)
{
	/* set the flag which will tell workers to stop */
	config->running = 0;
	/* running is just protected by a memory barrier */
	STARPU_WMB();
	starpu_wake_all_blocked_workers();
}

void starpu_display_stats()
{
	const char *stats;
	if ((stats = getenv("STARPU_BUS_STATS")) && atoi(stats))
		starpu_bus_profiling_helper_display_summary();

	if ((stats = getenv("STARPU_WORKER_STATS")) && atoi(stats))
		starpu_worker_profiling_helper_display_summary();
}
void starpu_shutdown(void)
{
	const char *stats;
	_STARPU_PTHREAD_MUTEX_LOCK(&init_mutex);
	init_count--;
	if (init_count)
	{
		_STARPU_DEBUG("Still somebody needing StarPU, don't deinitialize\n");
		_STARPU_PTHREAD_MUTEX_UNLOCK(&init_mutex);
		return;
	}

	/* We're last */
	initialized = CHANGING;
	_STARPU_PTHREAD_MUTEX_UNLOCK(&init_mutex);

	starpu_task_wait_for_no_ready();

	/* tell all workers to shutdown */
	_starpu_kill_all_workers(&config);

	{
	     int stats = starpu_get_env_number("STARPU_STATS");
	     if (stats != 0)
	     {
		  _starpu_display_msi_stats();
		  _starpu_display_alloc_cache_stats();
		  _starpu_display_comm_amounts();
	     }
	}

	{
	     int stats = starpu_get_env_number("STARPU_MEMORY_STATS");
	     if (stats != 0)
	     {
		  // Display statistics on data which have not been unregistered
		  starpu_memory_display_stats();
	     }
	}

	starpu_bus_profiling_helper_display_summary();
	starpu_worker_profiling_helper_display_summary();

	_starpu_deinitialize_registered_performance_models();

	/* wait for their termination */
	_starpu_terminate_workers(&config);

	_starpu_delete_all_sched_ctxs();

	_starpu_destroy_topology(&config);

#ifdef STARPU_USE_FXT
	_starpu_stop_fxt_profiling();
#endif

	_starpu_data_interface_shutdown();

	/* Drop all remaining tags */
	_starpu_tag_clear();

	_starpu_close_debug_logfile();

	_STARPU_PTHREAD_MUTEX_LOCK(&init_mutex);
	initialized = UNINITIALIZED;
	/* Let someone else that wants to initialize it again do it */
	_STARPU_PTHREAD_COND_SIGNAL(&init_cond);
	_STARPU_PTHREAD_MUTEX_UNLOCK(&init_mutex);

	/* Clear memory if it was allocated by StarPU */
	if (config.default_conf)
	     free(config.conf);

#ifdef HAVE_AYUDAME_H
	if (AYU_event) AYU_event(AYU_FINISH, 0, NULL);
#endif

	_STARPU_DEBUG("Shutdown finished\n");
}

unsigned starpu_worker_get_count(void)
{
	return config.topology.nworkers;
}

int starpu_worker_get_count_by_type(enum starpu_archtype type)
{
	switch (type)
	{
		case STARPU_CPU_WORKER:
			return config.topology.ncpus;

		case STARPU_CUDA_WORKER:
			return config.topology.ncudagpus;

		case STARPU_OPENCL_WORKER:
			return config.topology.nopenclgpus;

		case STARPU_GORDON_WORKER:
			return config.topology.ngordon_spus;

		default:
			return -EINVAL;
	}
}

unsigned starpu_combined_worker_get_count(void)
{
	return config.topology.ncombinedworkers;
}

unsigned starpu_cpu_worker_get_count(void)
{
	return config.topology.ncpus;
}

unsigned starpu_cuda_worker_get_count(void)
{
	return config.topology.ncudagpus;
}

unsigned starpu_opencl_worker_get_count(void)
{
	return config.topology.nopenclgpus;
}

unsigned starpu_spu_worker_get_count(void)
{
	return config.topology.ngordon_spus;
}

int starpu_asynchronous_copy_disabled(void)
{
	return config.conf->disable_asynchronous_copy;
}

int starpu_asynchronous_cuda_copy_disabled(void)
{
	return config.conf->disable_asynchronous_cuda_copy;
}

int starpu_asynchronous_opencl_copy_disabled(void)
{
	return config.conf->disable_asynchronous_opencl_copy;
}

/* When analyzing performance, it is useful to see what is the processing unit
 * that actually performed the task. This function returns the id of the
 * processing unit actually executing it, therefore it makes no sense to use it
 * within the callbacks of SPU functions for instance. If called by some thread
 * that is not controlled by StarPU, starpu_worker_get_id returns -1. */
int starpu_worker_get_id(void)
{
	struct _starpu_worker * worker;

	worker = _starpu_get_local_worker_key();
	if (worker)
	{
		return worker->workerid;
	}
	else
	{
		/* there is no worker associated to that thread, perhaps it is
		 * a thread from the application or this is some SPU worker */
		return -1;
	}
}

int starpu_combined_worker_get_id(void)
{
	struct _starpu_worker *worker;

	worker = _starpu_get_local_worker_key();
	if (worker)
	{
		return worker->combined_workerid;
	}
	else
	{
		/* there is no worker associated to that thread, perhaps it is
		 * a thread from the application or this is some SPU worker */
		return -1;
	}
}

int starpu_combined_worker_get_size(void)
{
	struct _starpu_worker *worker;

	worker = _starpu_get_local_worker_key();
	if (worker)
	{
		return worker->worker_size;
	}
	else
	{
		/* there is no worker associated to that thread, perhaps it is
		 * a thread from the application or this is some SPU worker */
		return -1;
	}
}

int starpu_combined_worker_get_rank(void)
{
	struct _starpu_worker *worker;

	worker = _starpu_get_local_worker_key();
	if (worker)
	{
		return worker->current_rank;
	}
	else
	{
		/* there is no worker associated to that thread, perhaps it is
		 * a thread from the application or this is some SPU worker */
		return -1;
	}
}

int starpu_worker_get_devid(int id)
{
	return config.workers[id].devid;
}

struct _starpu_worker *_starpu_get_worker_struct(unsigned id)
{
	return &config.workers[id];
}

unsigned starpu_worker_is_combined_worker(int id)
{
	return id >= (int)config.topology.nworkers;
}

struct _starpu_sched_ctx *_starpu_get_sched_ctx_struct(unsigned id)
{
        STARPU_ASSERT(id <= STARPU_NMAX_SCHED_CTXS);
	return &config.sched_ctxs[id];
}

struct _starpu_combined_worker *_starpu_get_combined_worker_struct(unsigned id)
{
	unsigned basic_worker_count = starpu_worker_get_count();

	STARPU_ASSERT(id >= basic_worker_count);
	return &config.combined_workers[id - basic_worker_count];
}

enum starpu_archtype starpu_worker_get_type(int id)
{
	return config.workers[id].arch;
}

int starpu_worker_get_ids_by_type(enum starpu_archtype type, int *workerids, int maxsize)
{
	unsigned nworkers = starpu_worker_get_count();

	int cnt = 0;

	unsigned id;
	for (id = 0; id < nworkers; id++)
	{
		if (starpu_worker_get_type(id) == type)
		{
			/* Perhaps the array is too small ? */
			if (cnt >= maxsize)
				return -ERANGE;

			workerids[cnt++] = id;
		}
	}

	return cnt;
}

void starpu_worker_get_name(int id, char *dst, size_t maxlen)
{
	char *name = config.workers[id].name;

	snprintf(dst, maxlen, "%s", name);
}

/* Retrieve the status which indicates what the worker is currently doing. */
enum _starpu_worker_status _starpu_worker_get_status(int workerid)
{
	return config.workers[workerid].status;
}

/* Change the status of the worker which indicates what the worker is currently
 * doing (eg. executing a callback). */
void _starpu_worker_set_status(int workerid, enum _starpu_worker_status status)
{
	config.workers[workerid].status = status;
}

int starpu_worker_get_nids_by_type(enum starpu_archtype type, int *workerids, int maxsize)
{
	unsigned nworkers = starpu_worker_get_count();

	int cnt = 0;

	unsigned id;
	for (id = 0; id < nworkers; id++)
	{
		if (starpu_worker_get_type(id) == type)
		{
			/* Perhaps the array is too small ? */
			if (cnt >= maxsize)
				return cnt;

			workerids[cnt++] = id;
		}
	}

	return cnt;
}

int starpu_worker_get_nids_ctx_free_by_type(enum starpu_archtype type, int *workerids, int maxsize)
{
	unsigned nworkers = starpu_worker_get_count();

	int cnt = 0;

	unsigned id, worker;
	unsigned found = 0;
	for (id = 0; id < nworkers; id++)
	{
		found = 0;
		if (starpu_worker_get_type(id) == type)
		{
			/* Perhaps the array is too small ? */
			if (cnt >= maxsize)
				return cnt;
			int s;
			for(s = 1; s < STARPU_NMAX_SCHED_CTXS; s++)
			{
				if(config.sched_ctxs[s].id != STARPU_NMAX_SCHED_CTXS)
				{
					struct worker_collection *workers = config.sched_ctxs[s].workers;
					if(workers->init_cursor)
						workers->init_cursor(workers);
					
					while(workers->has_next(workers))
					{
						worker = workers->get_next(workers);
						if(worker == id)
						{
							found = 1;
							break;
						}
					}
					
					if(workers->init_cursor)
						workers->deinit_cursor(workers);
					if(found) break;
				}
			}
			if(!found)
				workerids[cnt++] = id;
		}
	}

	return cnt;
}


struct _starpu_sched_ctx* _starpu_get_initial_sched_ctx(void)
{
	return &config.sched_ctxs[0];
}

int
starpu_driver_run(struct starpu_driver *d)
{
	if (!d)
		return -EINVAL;

	switch (d->type)
	{
#ifdef STARPU_USE_CPU
	case STARPU_CPU_WORKER:
		return _starpu_run_cpu(d);
#endif
#ifdef STARPU_USE_CUDA
	case STARPU_CUDA_WORKER:
		return _starpu_run_cuda(d);
#endif
#ifdef STARPU_USE_OPENCL
	case STARPU_OPENCL_WORKER:
		return _starpu_run_opencl(d);
#endif
	case STARPU_GORDON_WORKER: /* Not supported yet */
	default:
		return -EINVAL;
	}
}

int
starpu_driver_init(struct starpu_driver *d)
{
	STARPU_ASSERT(d);

	switch (d->type)
	{
#ifdef STARPU_USE_CPU
	case STARPU_CPU_WORKER:
		return _starpu_cpu_driver_init(d);
#endif
#ifdef STARPU_USE_CUDA
	case STARPU_CUDA_WORKER:
		return _starpu_cuda_driver_init(d);
#endif
#ifdef STARPU_USE_OPENCL
	case STARPU_OPENCL_WORKER:
		return _starpu_opencl_driver_init(d);
#endif
	case STARPU_GORDON_WORKER: /* Not supported yet */
	default:
		return -EINVAL;
	}
}

int
starpu_driver_run_once(struct starpu_driver *d)
{
	STARPU_ASSERT(d);

	switch (d->type)
	{
#ifdef STARPU_USE_CPU
	case STARPU_CPU_WORKER:
		return _starpu_cpu_driver_run_once(d);
#endif
#ifdef STARPU_USE_CUDA
	case STARPU_CUDA_WORKER:
		return _starpu_cuda_driver_run_once(d);
#endif
#ifdef STARPU_USE_OPENCL
	case STARPU_OPENCL_WORKER:
		return _starpu_opencl_driver_run_once(d);
#endif
	case STARPU_GORDON_WORKER: /* Not supported yet */
	default:
		return -EINVAL;
	}
}

int
starpu_driver_deinit(struct starpu_driver *d)
{
	STARPU_ASSERT(d);

	switch (d->type)
	{
#ifdef STARPU_USE_CPU
	case STARPU_CPU_WORKER:
		return _starpu_cpu_driver_deinit(d);
#endif
#ifdef STARPU_USE_CUDA
	case STARPU_CUDA_WORKER:
		return _starpu_cuda_driver_deinit(d);
#endif
#ifdef STARPU_USE_OPENCL
	case STARPU_OPENCL_WORKER:
		return _starpu_opencl_driver_deinit(d);
#endif
	case STARPU_GORDON_WORKER: /* Not supported yet */
	default:
		return -EINVAL;
	}
}
