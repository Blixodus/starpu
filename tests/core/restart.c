/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2009, 2010  Université de Bordeaux 1
 * Copyright (C) 2010  Centre National de la Recherche Scientifique
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

#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <starpu.h>
#include <stdlib.h>

#define N	10

struct timeval start;
struct timeval end;

int main(int argc, char **argv)
{
	unsigned iter;

	double init_timing = 0.0;
	double shutdown_timing = 0.0;

	for (iter = 0; iter < N; iter++)
	{
		gettimeofday(&start, NULL);
		/* Initialize StarPU */
		starpu_init(NULL);
		gettimeofday(&end, NULL);
		init_timing += (double)((end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec));

		gettimeofday(&start, NULL);
		/* Shutdown StarPU */
		starpu_shutdown();
		gettimeofday(&end, NULL);
		shutdown_timing += (double)((end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec));
	}

	fprintf(stderr, "starpu_init: %2.2f seconds\n", init_timing/(N*1000000));
	fprintf(stderr, "starpu_shutdown: %2.2f seconds\n", shutdown_timing/(N*1000000));

	return 0;
}
