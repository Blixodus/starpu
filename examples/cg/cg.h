/*
 * StarPU
 * Copyright (C) Université Bordeaux 1, CNRS 2008-2010 (see AUTHORS file)
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

#ifndef __STARPU_EXAMPLE_CG_H__
#define __STARPU_EXAMPLE_CG_H__

#include <starpu.h>
#include <math.h>
#include <common/blas.h>
#include <cuda.h>
#include <cublas.h>

#include <starpu.h>

#define DOUBLE

#ifdef DOUBLE
#define TYPE	double
#define GEMV	DGEMV
#define DOT	DDOT
#define GEMV	DGEMV
#define AXPY	DAXPY
#define SCAL	DSCAL
#define cublasdot	cublasDdot
#define cublasscal	cublasDscal
#define cublasaxpy	cublasDaxpy
#define cublasgemv	cublasDgemv
#else
#define TYPE	float
#define GEMV	SGEMV
#define DOT	SDOT
#define GEMV	SGEMV
#define AXPY	SAXPY
#define SCAL	SSCAL
#define cublasdot	cublasSdot
#define cublasscal	cublasSscal
#define cublasaxpy	cublasSaxpy
#define cublasgemv	cublasSgemv
#endif

void dot_kernel(starpu_data_handle v1,
                starpu_data_handle v2,
                starpu_data_handle s,
		unsigned nblocks);

void gemv_kernel(starpu_data_handle v1,
                starpu_data_handle matrix, 
                starpu_data_handle v2,
                TYPE p1, TYPE p2,
		unsigned nblocks);

void axpy_kernel(starpu_data_handle v1,
		starpu_data_handle v2, TYPE p1,
		unsigned nblocks);

void scal_axpy_kernel(starpu_data_handle v1, TYPE p1,
			starpu_data_handle v2, TYPE p2,
			unsigned nblocks);

void copy_handle(starpu_data_handle dst,
		starpu_data_handle src,
		unsigned nblocks);

#endif // __STARPU_EXAMPLE_CG_H__
