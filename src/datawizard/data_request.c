/*
 * StarPU
 * Copyright (C) INRIA 2008-2009 (see AUTHORS file)
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

#include <datawizard/data_request.h>
#include <pthread.h>

static data_request_list_t data_requests[MAXNODES];
static pthread_cond_t data_requests_list_cond[MAXNODES];
static pthread_mutex_t data_requests_list_mutex[MAXNODES];

void init_data_request_lists(void)
{
	unsigned i;
	for (i = 0; i < MAXNODES; i++)
	{
		data_requests[i] = data_request_list_new();
		pthread_mutex_init(&data_requests_list_mutex[i], NULL);
		pthread_cond_init(&data_requests_list_cond[i], NULL);
	}
}

void deinit_data_request_lists(void)
{
	unsigned i;
	for (i = 0; i < MAXNODES; i++)
	{
		pthread_cond_destroy(&data_requests_list_cond[i]);
		pthread_mutex_destroy(&data_requests_list_mutex[i]);
		data_request_list_delete(data_requests[i]);
	}
}

/* this should be called with the lock r->state->header_lock taken */
void data_request_destroy(data_request_t r)
{
	r->state->per_node[r->dst_node].request = NULL;

	data_request_delete(r);
}

/* state->lock should already be taken !  */
data_request_t create_data_request(data_state *state, uint32_t src_node, uint32_t dst_node, uint32_t handling_node, uint8_t read, uint8_t write)
{
	data_request_t r = data_request_new();

	starpu_spin_init(&r->lock);

	r->state = state;
	r->src_node = src_node;
	r->dst_node = dst_node;
	r->read = read;
	r->write = write;

	r->handling_node = handling_node;

	r->completed = 0;
	r->retval = -1;

	r->next_req = NULL;
	r->next_req_handler = 0;

	r->strictness = 1;

	/* associate that request with the state so that further similar
	 * requests will reuse that one  */
	state->per_node[dst_node].request = r;

	r->refcnt = 1;

	return r;
}

/* state->lock should be taken */
data_request_t try_to_reuse_a_data_request(data_state *state, uint32_t dst_node, uint8_t read, uint8_t write)
{
	data_request_t r = state->per_node[dst_node].request;

	if (r)
	{
		/* XXX perhaps this is too strict ! */
		STARPU_ASSERT(r->read == read);		
		STARPU_ASSERT(r->write == write);		

		r->refcnt++;
	}

	return r;
}

int wait_data_request_completion(data_request_t r)
{
	int retval;
	int do_delete = 0;

	uint32_t local_node = get_local_memory_node();

	do {
		starpu_spin_lock(&r->lock);

		if (r->completed)
			break;

		starpu_spin_unlock(&r->lock);

		wake_all_blocked_workers_on_node(r->handling_node);

		datawizard_progress(local_node);

	} while (1);

//	fprintf(stderr, "REQUEST COMPLETED !\n");

	retval = r->retval;

	r->refcnt--;

	/* if nobody is waiting on that request, we can get rid of it */
	if (r->refcnt == 0)
		do_delete = 1;

	starpu_spin_unlock(&r->lock);
	
	if (do_delete)
		data_request_destroy(r);
	
	return retval;
}

/* this is non blocking */
void post_data_request(data_request_t r, uint32_t handling_node)
{
//	fprintf(stderr, "POST REQUEST\n");

	/* insert the request in the proper list */
	pthread_mutex_lock(&data_requests_list_mutex[handling_node]);
	data_request_list_push_front(data_requests[handling_node], r);
	pthread_mutex_unlock(&data_requests_list_mutex[handling_node]);

	wake_all_blocked_workers_on_node(handling_node);
}

/* TODO : accounting to see how much time was spent working for other people ... */
static void handle_data_request(data_request_t r)
{
	unsigned do_delete = 0;
	
	starpu_spin_lock(&r->state->header_lock);

	starpu_spin_lock(&r->lock);

	//fprintf(stderr, "handle_data_request src %d dst %d state %p (read %d write %d)\n",  r->src_node, r->dst_node, r->state, r->read, r->write);

	
	//print_state_state("BEFORE handle_data_request ... ", r->state);

	/* perform the transfer */
	/* the header of the data must be locked by the worker that submitted the request */
	r->retval = driver_copy_data_1_to_1(r->state, r->src_node, r->dst_node, 0);

	update_data_state(r->state, r->dst_node, r->write);

	//print_state_state("AFTER handle_data_request ... ", r->state);

	r->completed = 1;
	
	/* if nobody is waiting on that request, we can get rid of it */
	if (r->refcnt == 0)
		do_delete = 1;
	
	starpu_spin_unlock(&r->lock);

	if (do_delete)
		data_request_destroy(r);

	starpu_spin_unlock(&r->state->header_lock);
}

void handle_node_data_requests(uint32_t src_node)
{
	pthread_mutex_lock(&data_requests_list_mutex[src_node]);

	/* for all entries of the list */
	data_request_list_t l = data_requests[src_node];
	data_request_t r;

	while (!data_request_list_empty(l))
	{
		r = data_request_list_pop_back(l);		
		pthread_mutex_unlock(&data_requests_list_mutex[src_node]);

		handle_data_request(r);
	
		/* wake the requesting worker up */
		pthread_mutex_lock(&data_requests_list_mutex[src_node]);

		// if we do not progress ..
		// pthread_cond_broadcast(&data_requests_list_cond[src_node]);
	}

	pthread_mutex_unlock(&data_requests_list_mutex[src_node]);
}
