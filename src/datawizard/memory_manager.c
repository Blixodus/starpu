/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2012-2013  Centre National de la Recherche Scientifique
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
#include <common/utils.h>
#include <common/thread.h>
#include <datawizard/memory_manager.h>

static size_t global_size[STARPU_MAXNODES];
static size_t used_size[STARPU_MAXNODES];
static starpu_pthread_mutex_t lock_nodes[STARPU_MAXNODES];

int _starpu_memory_manager_init()
{
	int i;

	for(i=0 ; i<STARPU_MAXNODES ; i++)
	{
		global_size[i] = 0;
		used_size[i] = 0;
		STARPU_PTHREAD_MUTEX_INIT(&lock_nodes[i], NULL);
	}
	return 0;
}

void _starpu_memory_manager_set_global_memory_size(unsigned node, size_t size)
{
	global_size[node] = size;
	_STARPU_DEBUG("Global size for node %d is %ld\n", node, (long)global_size[node]);
}

size_t _starpu_memory_manager_get_global_memory_size(unsigned node)
{
	return global_size[node];
}


int _starpu_memory_manager_can_allocate_size(size_t size, unsigned node)
{
	int ret;

	STARPU_PTHREAD_MUTEX_LOCK(&lock_nodes[node]);
	if (global_size[node] == 0)
	{
		// We do not have information on the available size, let's suppose it is going to fit
		used_size[node] += size;
		ret = 1;
	}
	else if (used_size[node] + size <= global_size[node])
	{
		used_size[node] += size;
		ret = 1;
	}
	else
	{
		ret = 0;
	}
	STARPU_PTHREAD_MUTEX_UNLOCK(&lock_nodes[node]);
	return ret;
}

void _starpu_memory_manager_deallocate_size(size_t size, unsigned node)
{
	STARPU_PTHREAD_MUTEX_LOCK(&lock_nodes[node]);
	used_size[node] -= size;
	STARPU_PTHREAD_MUTEX_UNLOCK(&lock_nodes[node]);
}

starpu_ssize_t starpu_memory_get_total(unsigned node)
{
	if (global_size[node] == 0)
		return -1;
	else
		return global_size[node];
}

starpu_ssize_t starpu_memory_get_available(unsigned node)
{
	if (global_size[node] == 0)
		return -1;
	else
		return global_size[node] - used_size[node];
}

int _starpu_memory_manager_test_allocate_size_(size_t size, unsigned node)
{
	int ret;

	STARPU_PTHREAD_MUTEX_LOCK(&lock_nodes[node]);
	if (global_size[node] == 0)
		ret = 1;
	else if (used_size[node] + size <= global_size[node])
		ret = 1;
	else
		ret = 0;
	STARPU_PTHREAD_MUTEX_UNLOCK(&lock_nodes[node]);
	return ret;
}
