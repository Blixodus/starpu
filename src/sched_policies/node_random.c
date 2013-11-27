/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2013  INRIA
 * Copyright (C) 2013  Simon Archipoff
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

#include <starpu_sched_node.h>
#include <core/workers.h>

static double compute_relative_speedup(struct starpu_sched_node * node)
{
	double sum = 0.0;
	int id;
	for(id = starpu_bitmap_first(node->workers_in_ctx);
	    id != -1;
	    id = starpu_bitmap_next(node->workers_in_ctx, id))
	{
		struct starpu_perfmodel_arch* perf_arch = starpu_worker_get_perf_archtype(id);
		sum += starpu_worker_get_relative_speedup(perf_arch);

	}
	STARPU_ASSERT(sum != 0.0);
	return sum;
}


static int random_push_task(struct starpu_sched_node * node, struct starpu_task * task)
{
	STARPU_ASSERT(node->nchilds > 0);

	/* indexes_nodes and size are used to memoize node that can execute tasks
	 * during the first phase of algorithm, it contain the size indexes of the nodes
	 * that can execute task.
	 */
	int indexes_nodes[node->nchilds];
	int size=0;

	/* speedup[i] is revelant only if i is in the size firsts elements of
	 * indexes_nodes
	 */
	double speedup[node->nchilds];

	double alpha_sum = 0.0;

	int i;
	for(i = 0; i < node->nchilds ; i++)
	{
		if(starpu_sched_node_can_execute_task(node->childs[i],task))
		{
			speedup[size] = compute_relative_speedup(node->childs[i]);
			alpha_sum += speedup[size];
			indexes_nodes[size] = i;
			size++;
		}
	}
	if(size == 0)
		return -ENODEV;

	/* not fully sure that this code is correct
	 * because of bad properties of double arithmetic
	 */
	double random = starpu_drand48()*alpha_sum;
	double alpha = 0.0;
	struct starpu_sched_node * select  = NULL;
	
	for(i = 0; i < size ; i++)
	{
		int index = indexes_nodes[i];
		if(alpha + speedup[i] >= random)
		{	
			select = node->childs[index];
			break;
		}
		alpha += speedup[i];
	}
	STARPU_ASSERT(select != NULL);
	int ret_val = select->push_task(select,task);

	return ret_val;
}
/* taking the min of estimated_end not seems to be a good value to return here
 * as random scheduler balance between childs very poorly
 */
double random_estimated_end(struct starpu_sched_node * node)
{
	double sum = 0.0;
	int i;
	for(i = 0; i < node->nchilds; i++)
		sum += node->childs[i]->estimated_end(node->childs[i]);
	return sum / node->nchilds;
}

struct starpu_sched_node * starpu_sched_node_random_create(void * arg STARPU_ATTRIBUTE_UNUSED)
{
	struct starpu_sched_node * node = starpu_sched_node_create();
	node->estimated_end = random_estimated_end;
	node->push_task = random_push_task;
	return node;
}

int starpu_sched_node_is_random(struct starpu_sched_node *node)
{
	return node->push_task == random_push_task;
}
