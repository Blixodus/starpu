/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2010-2014  Université de Bordeaux
 * Copyright (C) 2010, 2011, 2012, 2013  Centre National de la Recherche Scientifique
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

#include <starpu.h>
#include <common/thread.h>

#include "../helper.h"

#ifdef STARPU_QUICK_CHECK
static unsigned niter = 64;
#else
static unsigned niter = 16384;
#endif

/*
 *
 *		    /-->B--\
 *		    |      |
 *	     -----> A      D---\--->
 *		^   |      |   |
 *		|   \-->C--/   |
 *		|              |
 *		\--------------/
 *
 *	- {B, C} depend on A
 *	- D depends on {B, C}
 *	- A, B, C and D are resubmitted at the end of the loop (or not)
 */

static struct starpu_task taskA, taskB, taskC, taskD;

static unsigned loop_cnt = 0;
static unsigned *check_cnt;
static starpu_pthread_cond_t cond = STARPU_PTHREAD_COND_INITIALIZER;
static starpu_pthread_mutex_t mutex = STARPU_PTHREAD_MUTEX_INITIALIZER;

extern void cuda_host_increment(void *descr[], void *_args);

void cpu_increment(void *descr[], void *arg STARPU_ATTRIBUTE_UNUSED)
{
	unsigned *var = (unsigned *)STARPU_VARIABLE_GET_PTR(descr[0]);
	(*var)++;
}

static struct starpu_codelet dummy_codelet =
{
	.cpu_funcs = {cpu_increment},
#ifdef STARPU_USE_CUDA
	.cuda_funcs = {cuda_host_increment},
	.cuda_flags = {STARPU_CUDA_ASYNC},
#endif
	// TODO
	//.opencl_funcs = {dummy_func},
	.cpu_funcs_name = {"cpu_increment"},
	.model = NULL,
	.modes = { STARPU_RW },
	.nbuffers = 1
};

static void callback_task_D(void *arg STARPU_ATTRIBUTE_UNUSED)
{
	STARPU_PTHREAD_MUTEX_LOCK(&mutex);
	loop_cnt++;

	if (loop_cnt == niter)
	{
		/* We are done */
		taskD.regenerate = 0;
		STARPU_PTHREAD_COND_SIGNAL(&cond);
		STARPU_PTHREAD_MUTEX_UNLOCK(&mutex);
	}
	else
	{
		int ret;
		if (loop_cnt == niter-1)
		{
			taskB.regenerate = 0;
			taskC.regenerate = 0;
		}
		STARPU_PTHREAD_MUTEX_UNLOCK(&mutex);
		/* Let's go for another iteration */
		ret = starpu_task_submit(&taskA);
		STARPU_CHECK_RETURN_VALUE(ret, "starpu_task_submit");
	}
}

int main(int argc, char **argv)
{
//	unsigned i;
//	double timing;
//	double start;
//	double end;
	int ret;

	ret = starpu_initialize(NULL, &argc, &argv);
	if (ret == -ENODEV) return STARPU_TEST_SKIPPED;
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");

	/* Implicit data dependencies and regeneratable tasks are not compatible */
	starpu_data_set_default_sequential_consistency_flag(0);

	starpu_malloc((void**)&check_cnt, sizeof(*check_cnt));
	*check_cnt = 0;

	starpu_data_handle_t check_data;
	starpu_variable_data_register(&check_data, STARPU_MAIN_RAM, (uintptr_t)check_cnt, sizeof(*check_cnt));

	starpu_task_init(&taskA);
	taskA.cl = &dummy_codelet;
	taskA.cl_arg = &taskA;
	taskA.cl_arg_size = sizeof(&taskA);
	taskA.regenerate = 0; /* this task will be explicitely resubmitted if needed */
	taskA.handles[0] = check_data;

	starpu_task_init(&taskB);
	taskB.cl = &dummy_codelet;
	taskB.cl_arg = &taskB;
	taskB.cl_arg_size = sizeof(&taskB);
	taskB.regenerate = 1;
	taskB.handles[0] = check_data;

	starpu_task_init(&taskC);
	taskC.cl = &dummy_codelet;
	taskC.cl_arg = &taskC;
	taskC.cl_arg_size = sizeof(&taskC);
	taskC.regenerate = 1;
	taskC.handles[0] = check_data;

	starpu_task_init(&taskD);
	taskD.cl = &dummy_codelet;
	taskD.cl_arg = &taskD;
	taskD.cl_arg_size = sizeof(&taskD);
	taskD.callback_func = callback_task_D;
	taskD.regenerate = 1;
	taskD.handles[0] = check_data;

	struct starpu_task *depsBC_array[1] = {&taskA};
	starpu_task_declare_deps_array(&taskB, 1, depsBC_array);
	starpu_task_declare_deps_array(&taskC, 1, depsBC_array);

	struct starpu_task *depsD_array[2] = {&taskB, &taskC};
	starpu_task_declare_deps_array(&taskD, 2, depsD_array);

	ret = starpu_task_submit(&taskA); if (ret == -ENODEV) goto enodev; STARPU_CHECK_RETURN_VALUE(ret, "starpu_task_submit");
	ret = starpu_task_submit(&taskB); if (ret == -ENODEV) goto enodev; STARPU_CHECK_RETURN_VALUE(ret, "starpu_task_submit");
	ret = starpu_task_submit(&taskC); if (ret == -ENODEV) goto enodev; STARPU_CHECK_RETURN_VALUE(ret, "starpu_task_submit");
	ret = starpu_task_submit(&taskD); if (ret == -ENODEV) goto enodev; STARPU_CHECK_RETURN_VALUE(ret, "starpu_task_submit");

	/* Wait for the termination of all loops */
	STARPU_PTHREAD_MUTEX_LOCK(&mutex);
	if (loop_cnt < niter)
		STARPU_PTHREAD_COND_WAIT(&cond, &mutex);
	STARPU_PTHREAD_MUTEX_UNLOCK(&mutex);

	starpu_data_acquire(check_data, STARPU_R);
	starpu_data_release(check_data);

	STARPU_ASSERT(*check_cnt == (4*loop_cnt));

	starpu_free(check_cnt);
	starpu_data_unregister(check_data);

	starpu_shutdown();

	/* Cleanup the statically allocated tasks after shutdown, as StarPU is still working on it after the callback */
	starpu_task_clean(&taskA);
	starpu_task_clean(&taskB);
	starpu_task_clean(&taskC);
	starpu_task_clean(&taskD);

	return EXIT_SUCCESS;

enodev:
	fprintf(stderr, "WARNING: No one can execute this task\n");
	/* yes, we do not perform the computation but we did detect that no one
 	 * could perform the kernel, so this is not an error from StarPU */
	starpu_data_unregister(check_data);
	starpu_shutdown();
	return STARPU_TEST_SKIPPED;
}

