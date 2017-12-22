/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2010-2013,2015-2017                      CNRS
 * Copyright (C) 2010,2012,2014-2016                      Université de Bordeaux
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

#include <starpu_mpi_private.h>

int _starpu_debug_rank=-1;
int _starpu_debug_level_min=0;
int _starpu_debug_level_max=0;
int _starpu_mpi_tag = 42;
int _starpu_mpi_comm_debug;

void _starpu_mpi_set_debug_level_min(int level)
{
	_starpu_debug_level_min = level;
}

void _starpu_mpi_set_debug_level_max(int level)
{
	_starpu_debug_level_max = level;
}

int starpu_mpi_get_communication_tag(void)
{
	return _starpu_mpi_tag;
}

void starpu_mpi_set_communication_tag(int tag)
{
	_starpu_mpi_tag = tag;
}

char *_starpu_mpi_get_mpi_error_code(int code)
{
	static char str[MPI_MAX_OBJECT_NAME];
	int len;
	MPI_Error_string(code, str, &len);
	return str;
}
