/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2009, 2010  Université de Bordeaux 1
 * Copyright (C) 2010, 2011, 2012  Centre National de la Recherche Scientifique
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

#ifndef __MPI_CHOLESKY_H__
#define __MPI_CHOLESKY_H__

#include <string.h>
#include <math.h>
#include <sys/time.h>
#ifdef STARPU_USE_CUDA
#include <cuda.h>
#include <cuda_runtime.h>
#include <cublas.h>
#endif

#include <common/blas.h>
#include <starpu.h>

#define BLOCKSIZE	(size/nblocks)

static unsigned size = 4*1024;
static unsigned nblocks = 16;
static unsigned nbigblocks = 2;
static unsigned noprio = 0;
static unsigned display = 0;
static unsigned dblockx = -1;
static unsigned dblocky = -1;

void chol_cpu_codelet_update_u11(void **, void *);
void chol_cpu_codelet_update_u21(void **, void *);
void chol_cpu_codelet_update_u22(void **, void *);

#ifdef STARPU_USE_CUDA
void chol_cublas_codelet_update_u11(void *descr[], void *_args);
void chol_cublas_codelet_update_u21(void *descr[], void *_args);
void chol_cublas_codelet_update_u22(void *descr[], void *_args);
#endif

static void __attribute__((unused)) parse_args(int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-size") == 0)
		{
		        char *argptr;
			size = strtol(argv[++i], &argptr, 10);
		}

		if (strcmp(argv[i], "-dblockx") == 0)
		{
		        char *argptr;
			dblockx = strtol(argv[++i], &argptr, 10);
		}
		
		if (strcmp(argv[i], "-dblocky") == 0)
		{
		        char *argptr;
			dblocky = strtol(argv[++i], &argptr, 10);
		}
	
		if (strcmp(argv[i], "-nblocks") == 0)
		{
		        char *argptr;
			nblocks = strtol(argv[++i], &argptr, 10);
		}

		if (strcmp(argv[i], "-nbigblocks") == 0)
		{
		        char *argptr;
			nbigblocks = strtol(argv[++i], &argptr, 10);
		}

		if (strcmp(argv[i], "-no-prio") == 0)
		{
			noprio = 1;
		}

		if (strcmp(argv[i], "-display") == 0)
		{
			display = 1;
		}

		if (strcmp(argv[i], "-h") == 0)
		{
			printf("usage : %s [-display] [-size size] [-nblocks nblocks]\n", argv[0]);
		}
	}
	if (nblocks > size) nblocks = size;
}

#endif // __MPI_CHOLESKY_H__
