/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2013  Université de Bordeaux 1
 * Copyright (C) 2012-2013  Centre National de la Recherche Scientifique
 * Copyright (C) 2011-2013  INRIA
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
#ifdef STARPU_HAVE_HWLOC
#include <hwloc.h>
#include "core/workers.h"

static unsigned tree_has_next(struct starpu_worker_collection *workers, struct starpu_sched_ctx_iterator *it)
{
	STARPU_ASSERT(it != NULL);
	if(workers->nworkers == 0)
		return 0;

	struct starpu_tree *tree = (struct starpu_tree*)workers->workerids;
	struct starpu_tree *neighbour = starpu_tree_get_neighbour(tree, (struct starpu_tree*)it->value, it->visited, workers->present);
	
	if(!neighbour)
	{
		starpu_tree_reset_visited(tree, it->visited);
		it->value = NULL;
		it->possible_value = NULL;
		return 0;
	}

	it->possible_value = neighbour;
	int id = _starpu_worker_get_workerid(neighbour->id);

	STARPU_ASSERT_MSG(id != -1, "bind id (%d) for workerid (%d) not correct", neighbour->id, id);

	return 1;
}

static int tree_get_next(struct starpu_worker_collection *workers, struct starpu_sched_ctx_iterator *it)
{
	int ret = -1;
	
	struct starpu_tree *tree = (struct starpu_tree *)workers->workerids;
	struct starpu_tree *neighbour = NULL;
	if(it->possible_value)
	{
		neighbour = it->possible_value;
		it->possible_value = NULL;
	}
	else
		neighbour = starpu_tree_get_neighbour(tree, (struct starpu_tree*)it->value, it->visited, workers->present);
	
	STARPU_ASSERT_MSG(neighbour, "no element anymore");
	
	it->value = neighbour;

	ret = _starpu_worker_get_workerid(neighbour->id);
	STARPU_ASSERT_MSG(ret != -1, "bind id not correct");
	it->visited[neighbour->id] = 1;

	return ret;
}

static int tree_add(struct starpu_worker_collection *workers, int worker)
{
	struct starpu_tree *tree = (struct starpu_tree *)workers->workerids;

	int bindid = starpu_worker_get_bindid(worker);
	if(!workers->present[bindid])
	{
		workers->present[bindid] = 1;
		workers->nworkers++;
		return worker;
	}
	else 
		return -1;
}


static int tree_remove(struct starpu_worker_collection *workers, int worker)
{
	struct starpu_tree *tree = (struct starpu_tree *)workers->workerids;

	int bindid = starpu_worker_get_bindid(worker);
	if(workers->present[bindid])
	{
		workers->present[bindid] = 0;
		workers->nworkers--;
		return worker;
	}
	else 
		return -1;
}

static void tree_init(struct starpu_worker_collection *workers)
{
	workers->workerids = (void*)starpu_workers_get_tree();
	workers->nworkers = 0;
	
	int i;
	for(i = 0; i < STARPU_NMAXWORKERS; i++)
		workers->present[i] = 0;
	
	return;
}

static void tree_deinit(struct starpu_worker_collection *workers)
{
//	free(workers->workerids);
}

static void tree_init_iterator(struct starpu_worker_collection *workers STARPU_ATTRIBUTE_UNUSED, struct starpu_sched_ctx_iterator *it)
{
	it->value = NULL;
	it->possible_value = NULL;
	int i;
	for(i = 0; i < STARPU_NMAXWORKERS; i++)
		it->visited[i] = 0;
}

struct starpu_worker_collection worker_tree =
{
	.has_next = tree_has_next,
	.get_next = tree_get_next,
	.add = tree_add,
	.remove = tree_remove,
	.init = tree_init,
	.deinit = tree_deinit,
	.init_iterator = tree_init_iterator,
	.type = STARPU_WORKER_TREE
};

#endif// STARPU_HAVE_HWLOC
