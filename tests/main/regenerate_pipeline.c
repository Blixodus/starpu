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

#include <stdio.h>
#include <unistd.h>
#include <starpu.h>
#include "../helper.h"
#include <common/thread.h>

#ifdef STARPU_QUICK_CHECK
static unsigned ntasks = 64;
#else
static unsigned ntasks = 65536;
#endif
static unsigned cntA = 0;
static unsigned cntB = 0;
static unsigned cntC = 0;

static unsigned completed = 0;
static starpu_pthread_mutex_t mutex = STARPU_PTHREAD_MUTEX_INITIALIZER;
static starpu_pthread_cond_t cond = STARPU_PTHREAD_COND_INITIALIZER;

static
void callback(void *arg)
{
	struct starpu_task *task = starpu_task_get_current();
	unsigned *cnt = arg;
	unsigned res;

	res = STARPU_ATOMIC_ADD(cnt, 1);

	if (res == ntasks)
	{
		task->regenerate = 0;
		FPRINTF(stderr, "Stop !\n");

		STARPU_PTHREAD_MUTEX_LOCK(&mutex);
		completed = 1;
		STARPU_PTHREAD_COND_SIGNAL(&cond);
		STARPU_PTHREAD_MUTEX_UNLOCK(&mutex);
	}
}

void dummy_func(void *descr[] STARPU_ATTRIBUTE_UNUSED, void *arg STARPU_ATTRIBUTE_UNUSED)
{
}

static struct starpu_codelet dummy_codelet = 
{
	.cpu_funcs = {dummy_func},
	.cuda_funcs = {dummy_func},
	.opencl_funcs = {dummy_func},
	.cpu_funcs_name = {"dummy_func"},
	.model = NULL,
	.nbuffers = 0
};

static void parse_args(int argc, char **argv)
{
	int c;
	while ((c = getopt(argc, argv, "i:")) != -1)
	switch(c)
	{
		case 'i':
			ntasks = atoi(optarg);
			break;
	}
}

int main(int argc, char **argv)
{
	//	unsigned i;
	double timing;
	double start;
	double end;
	int ret;

	parse_args(argc, argv);

	ret = starpu_initialize(NULL, &argc, &argv);
	if (ret == -ENODEV) return STARPU_TEST_SKIPPED;
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");

	struct starpu_task taskA, taskB, taskC;
	struct starpu_task *taskAp = &taskA;
	struct starpu_task *taskBp = &taskB;

	starpu_task_init(&taskA);
	taskA.cl = &dummy_codelet;
	taskA.regenerate = 1;
	taskA.detach = 1;
	taskA.callback_func = callback;
	taskA.callback_arg = &cntA;

	starpu_task_init(&taskB);
	taskB.cl = &dummy_codelet;
	taskB.regenerate = 1;
	taskB.detach = 1;
	taskB.callback_func = callback;
	taskB.callback_arg = &cntB;

	starpu_task_declare_deps_array(&taskB, 1, &taskAp);

	starpu_task_init(&taskC);
	taskC.cl = &dummy_codelet;
	taskC.regenerate = 1;
	taskC.detach = 1;
	taskC.callback_func = callback;
	taskC.callback_arg = &cntC;
	starpu_task_declare_deps_array(&taskA, 1, &taskBp);

	FPRINTF(stderr, "#tasks : %u\n", ntasks);

	start = starpu_timing_now();

	ret = starpu_task_submit(&taskA);
	if (ret == -ENODEV) goto enodev;
	ret = starpu_task_submit(&taskB);
	if (ret == -ENODEV) goto enodev;
	ret = starpu_task_submit(&taskC);
	if (ret == -ENODEV) goto enodev;
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_task_submit");

	STARPU_PTHREAD_MUTEX_LOCK(&mutex);
	if (!completed)
		STARPU_PTHREAD_COND_WAIT(&cond, &mutex);
	STARPU_PTHREAD_MUTEX_UNLOCK(&mutex);

	end = starpu_timing_now();

	timing = end - start;

	FPRINTF(stderr, "Total: %f secs\n", timing/1000000);
	FPRINTF(stderr, "Per task: %f usecs\n", timing/ntasks);

	starpu_shutdown();

	/* Cleanup the statically allocated tasks after shutdown, as StarPU is still working on it after the callback */
	starpu_task_clean(&taskA);
	starpu_task_clean(&taskB);

	return EXIT_SUCCESS;

enodev:
	fprintf(stderr, "WARNING: No one can execute this task\n");
	/* yes, we do not perform the computation but we did detect that no one
 	 * could perform the kernel, so this is not an error from StarPU */
	starpu_shutdown();
	return STARPU_TEST_SKIPPED;
}
