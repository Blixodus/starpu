/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2010-2012  Université de Bordeaux
 * Copyright (C) 2010, 2011, 2012, 2013  CNRS
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

#ifndef __STARPU_CUBLAS_V2_H__
#define __STARPU_CUBLAS_V2_H__

#include <cublas_v2.h>

#ifdef __cplusplus
extern "C"
{
#endif

cublasHandle_t starpu_cublas_get_local_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* __STARPU_CUBLAS_V2_H__ */
