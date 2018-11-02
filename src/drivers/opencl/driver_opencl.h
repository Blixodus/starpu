/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2010,2011,2013,2014,2018                 Université de Bordeaux
 * Copyright (C) 2012                                     Inria
 * Copyright (C) 2010,2012,2015,2017                      CNRS
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

#ifndef __DRIVER_OPENCL_H__
#define __DRIVER_OPENCL_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#ifdef STARPU_USE_OPENCL

#define CL_TARGET_OPENCL_VERSION 100
#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#endif

#if defined(STARPU_USE_OPENCL) || defined(STARPU_SIMGRID)
struct _starpu_machine_config;
void _starpu_opencl_discover_devices(struct _starpu_machine_config *config);

unsigned _starpu_opencl_get_device_count(void);
void _starpu_opencl_init(void);
void *_starpu_opencl_worker(void *);
#else
#define _starpu_opencl_discover_devices(config) ((void) (config))
#endif

#ifdef STARPU_USE_OPENCL
extern struct _starpu_driver_ops _starpu_driver_opencl_ops;
extern char *_starpu_opencl_program_dir;

int _starpu_run_opencl(struct _starpu_worker *);
int _starpu_opencl_driver_init(struct _starpu_worker *);
int _starpu_opencl_driver_run_once(struct _starpu_worker *);
int _starpu_opencl_driver_deinit(struct _starpu_worker *);

int _starpu_opencl_init_context(int devid);
int _starpu_opencl_deinit_context(int devid);
cl_device_type _starpu_opencl_get_device_type(int devid);

uintptr_t
_starpu_opencl_map_ram(uintptr_t src_ptr, size_t src_offset, unsigned src_node,
		      unsigned dst_node, size_t size, int *ret);
int
_starpu_opencl_unmap_ram(uintptr_t src_ptr, size_t src_offset, unsigned src_node,
		      uintptr_t dst_ptr, unsigned dst_node, size_t size);

int
_starpu_opencl_update_opencl_map(uintptr_t src, size_t src_offset, unsigned src_node STARPU_ATTRIBUTE_UNUSED,
			      uintptr_t dst, unsigned dst_node STARPU_ATTRIBUTE_UNUSED,
			      size_t size);
int
_starpu_opencl_update_cpu_map(uintptr_t src, unsigned src_node STARPU_ATTRIBUTE_UNUSED,
			      uintptr_t dst STARPU_ATTRIBUTE_UNUSED, size_t dst_offset STARPU_ATTRIBUTE_UNUSED, unsigned dst_node STARPU_ATTRIBUTE_UNUSED,
			      size_t size);
#endif

#if 0
cl_int _starpu_opencl_copy_rect_opencl_to_ram(cl_mem buffer, unsigned src_node, void *ptr, unsigned dst_node, const size_t buffer_origin[3], const size_t host_origin[3],
                                              const size_t region[3], size_t buffer_row_pitch, size_t buffer_slice_pitch,
                                              size_t host_row_pitch, size_t host_slice_pitch, cl_event *event);

cl_int _starpu_opencl_copy_rect_ram_to_opencl(void *ptr, unsigned src_node, cl_mem buffer, unsigned dst_node, const size_t buffer_origin[3], const size_t host_origin[3],
                                              const size_t region[3], size_t buffer_row_pitch, size_t buffer_slice_pitch,
                                              size_t host_row_pitch, size_t host_slice_pitch, cl_event *event);
#endif

#endif //  __DRIVER_OPENCL_H__
