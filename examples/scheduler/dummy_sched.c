/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2010-2012  Université de Bordeaux 1
 * Copyright (C) 2010-2012  Centre National de la Recherche Scientifique
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

#include <pthread.h>
#include <starpu.h>

#define NTASKS	32000
#define FPRINTF(ofile, fmt, args ...) do { if (!getenv("STARPU_SSILENT")) {fprintf(ofile, fmt, ##args); }} while(0)

typedef struct dummy_sched_data {
	struct starpu_task_list sched_list;
	pthread_mutex_t sched_mutex;
	pthread_cond_t sched_cond;
} dummy_sched_data;

static void dummy_sched_add_workers(unsigned sched_ctx_id, int *workerids, unsigned nworkers)
{
	struct dummy_sched_data *data = (struct dummy_sched_data*)starpu_sched_ctx_get_policy_data(sched_ctx_id);
	
	unsigned i;
	int workerid;
	for(i = 0; i < nworkers; i++)
	{
		workerid = workerids[i];
		starpu_worker_set_sched_condition(sched_ctx_id, workerid, &data->sched_mutex,  &data->sched_cond);
	}
}

static void dummy_sched_remove_workers(unsigned sched_ctx_id, int *workerids, unsigned nworkers)
{
	unsigned i;
	int workerid;
	for(i = 0; i < nworkers; i++)
	{
		workerid = workerids[i];
		starpu_worker_set_sched_condition(sched_ctx_id, workerid, NULL,  NULL);
	}
}

static void init_dummy_sched(unsigned sched_ctx_id)
{
	starpu_create_worker_collection_for_sched_ctx(sched_ctx_id, WORKER_LIST);

	struct dummy_sched_data *data = (struct dummy_sched_data*)malloc(sizeof(struct dummy_sched_data));
	

	/* Create a linked-list of tasks and a condition variable to protect it */
	starpu_task_list_init(&data->sched_list);

	pthread_mutex_init(&data->sched_mutex, NULL);
	pthread_cond_init(&data->sched_cond, NULL);

	starpu_sched_ctx_set_policy_data(sched_ctx_id, (void*)data);		

	FPRINTF(stderr, "Initialising Dummy scheduler\n");
}

static void deinit_dummy_sched(unsigned sched_ctx_id)
{
	struct dummy_sched_data *data = (struct dummy_sched_data*)starpu_sched_ctx_get_policy_data(sched_ctx_id);

	STARPU_ASSERT(starpu_task_list_empty(&data->sched_list));

	pthread_cond_destroy(&data->sched_cond);
	pthread_mutex_destroy(&data->sched_mutex);

	starpu_delete_worker_collection_for_sched_ctx(sched_ctx_id);

	free(data);
	
	FPRINTF(stderr, "Destroying Dummy scheduler\n");
}

static int push_task_dummy(struct starpu_task *task)
{
	unsigned sched_ctx_id = task->sched_ctx;
	struct dummy_sched_data *data = (struct dummy_sched_data*)starpu_sched_ctx_get_policy_data(sched_ctx_id);

	pthread_mutex_lock(&data->sched_mutex);

	starpu_task_list_push_front(&data->sched_list, task);

	pthread_cond_signal(&data->sched_cond);

	pthread_mutex_unlock(&data->sched_mutex);

	return 0;
}

/* The mutex associated to the calling worker is already taken by StarPU */
static struct starpu_task *pop_task_dummy(unsigned sched_ctx_id)
{
	/* NB: In this simplistic strategy, we assume that all workers are able
	 * to execute all tasks, otherwise, it would have been necessary to go
	 * through the entire list until we find a task that is executable from
	 * the calling worker. So we just take the head of the list and give it
	 * to the worker. */
	struct dummy_sched_data *data = (struct dummy_sched_data*)starpu_sched_ctx_get_policy_data(sched_ctx_id);
	return starpu_task_list_pop_back(&data->sched_list);
}

static struct starpu_sched_policy dummy_sched_policy =
{
	.init_sched = init_dummy_sched,
	.add_workers = dummy_sched_add_workers,
	.remove_workers = dummy_sched_remove_workers,
	.deinit_sched = deinit_dummy_sched,
	.push_task = push_task_dummy,
	.pop_task = pop_task_dummy,
	.post_exec_hook = NULL,
	.pop_every_task = NULL,
	.policy_name = "dummy",
	.policy_description = "dummy scheduling strategy"
};

static void dummy_func(void *descr[] __attribute__ ((unused)), void *arg __attribute__ ((unused)))
{
}

static struct starpu_codelet dummy_codelet =
{
	.where = STARPU_CPU|STARPU_CUDA|STARPU_OPENCL,
	.cpu_funcs = {dummy_func, NULL},
	.cuda_funcs = {dummy_func, NULL},
        .opencl_funcs = {dummy_func, NULL},
	.model = NULL,
	.nbuffers = 0
};


int main(int argc, char **argv)
{
	int ntasks = NTASKS;
	int ret;
	struct starpu_conf conf;

	starpu_conf_init(&conf);
	conf.sched_policy = &dummy_sched_policy,
	ret = starpu_init(&conf);
	if (ret == -ENODEV)
		return 77;
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");

#ifdef STARPU_QUICK_CHECK
	ntasks /= 100;
#endif

	unsigned i;
	for (i = 0; i < ntasks; i++)
	{
		struct starpu_task *task = starpu_task_create();
	
		task->cl = &dummy_codelet;
		task->cl_arg = NULL;
	
		ret = starpu_task_submit(task);
		STARPU_CHECK_RETURN_VALUE(ret, "starpu_task_submit");
	}

	starpu_task_wait_for_all();

	starpu_shutdown();

	return 0;
}
