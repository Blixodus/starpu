/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2016  Université de Bordeaux
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
 * This is a dumb sample of stencil application
 *
 * Dumb domain split in N pieces:
 *
 * 0 | 1 | ... | N-1
 *
 * for each simulation iteration, a task works on some adjactent pieces
 *
 * Locality is thus set on the central piece.
 */

#include <starpu.h>
#include "../helper.h"

#define N 50

#define ITER 50

int task_worker[N][ITER];

void cpu_f(void *descr[] STARPU_ATTRIBUTE_UNUSED, void *_args)
{
	unsigned i, loop;
	starpu_codelet_unpack_args(_args, &loop, &i);
	task_worker[i][loop] = starpu_worker_get_id();
	starpu_sleep(0.001);
}

static struct starpu_codelet cl =
{
	.cpu_funcs = { cpu_f },
	.cpu_funcs_name = { "cpu_f" },
	.nbuffers = 3,
	.modes =
	{
		STARPU_RW | STARPU_COMMUTE | STARPU_LOCALITY,
		STARPU_RW | STARPU_COMMUTE | STARPU_LOCALITY,
		STARPU_RW | STARPU_COMMUTE | STARPU_LOCALITY,
	},
};

int main(int argc, char *argv[])
{
	int ret;
	starpu_data_handle_t A[N];
	unsigned i, loop;

	ret = starpu_initialize(NULL, &argc, &argv);
	if (ret == -ENODEV) return STARPU_TEST_SKIPPED;
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");

	/* Get most parallelism by using an arbiter */
	starpu_arbiter_t arbiter = starpu_arbiter_create();
	for (i = 0; i < N; i++)
	{
		starpu_void_data_register(&A[i]);
		starpu_data_assign_arbiter(A[i], arbiter);
	}

	for (loop = 0; loop < ITER; loop++)
	{
		for (i = 1; i < N-1; i++)
		{
			starpu_task_insert(&cl,
					STARPU_RW | STARPU_COMMUTE | STARPU_LOCALITY, A[i-1],
					STARPU_RW | STARPU_COMMUTE | STARPU_LOCALITY, A[i],
					STARPU_RW | STARPU_COMMUTE | STARPU_LOCALITY, A[i+1],
					STARPU_VALUE, &loop, sizeof(loop),
					STARPU_VALUE, &i, sizeof(i),
					0);
		}
	}

	starpu_task_wait_for_all();

	for (loop = 0; loop < ITER; loop++)
	{
		for (i = 1; i < N-1; i++)
		{
			printf("%d ", task_worker[i][loop]);
		}
		printf("\n");
	}

	starpu_shutdown();
	return EXIT_SUCCESS;
}
