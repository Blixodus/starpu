/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2012-2021, 2023  Université de Bordeaux, CNRS (LaBRI UMR 5800), Inria
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

/* Use the "double" type */
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void complex_copy_opencl(__global double *output,
				  unsigned output_offset,
				  __global double *input,
				  unsigned input_offset,
				  unsigned nx)
{
        const int i = get_global_id(0);
        if (i < nx)
	{
		output = (__global char*) output + output_offset;
		input = (__global char*) input + input_offset;

		output[i] = input[i];
        }
}
