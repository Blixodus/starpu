/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2010, 2012-2014  Université de Bordeaux
 * Copyright (C) 2010, 2011, 2012, 2013, 2014  Centre National de la Recherche Scientifique
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

#ifndef __STARPU_MPI_PRIVATE_H__
#define __STARPU_MPI_PRIVATE_H__

#include <starpu.h>
#include <common/config.h>
#include <common/uthash.h>
#include "starpu_mpi.h"
#include "starpu_mpi_fxt.h"
#include <common/list.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int _debug_rank;

#ifdef STARPU_VERBOSE
extern int _debug_level_min;
extern int _debug_level_max;
void _starpu_mpi_set_debug_level_min(int level);
void _starpu_mpi_set_debug_level_max(int level);
#endif

#ifdef STARPU_VERBOSE
#  define _STARPU_MPI_DEBUG(level, fmt, ...) \
	do \
	{								\
		if (!getenv("STARPU_SILENT") && _debug_level_min <= level && level <= _debug_level_max)	\
		{							\
			if (_debug_rank == -1) MPI_Comm_rank(MPI_COMM_WORLD, &_debug_rank); \
			fprintf(stderr, "%*s[%d][starpu_mpi][%s:%d] " fmt , (_debug_rank+1)*4, "", _debug_rank, __starpu_func__ , __LINE__,## __VA_ARGS__); \
			fflush(stderr); \
		}			\
	} while(0);
#else
#  define _STARPU_MPI_DEBUG(level, fmt, ...)
#endif

#define _STARPU_MPI_DISP(fmt, ...) do { if (!getenv("STARPU_SILENT")) { \
	       				     if (_debug_rank == -1) MPI_Comm_rank(MPI_COMM_WORLD, &_debug_rank); \
                                             fprintf(stderr, "%*s[%d][starpu_mpi][%s:%d] " fmt , (_debug_rank+1)*4, "", _debug_rank, __starpu_func__ , __LINE__ ,## __VA_ARGS__); \
                                             fflush(stderr); }} while(0);

#ifdef STARPU_VERBOSE0
#  define _STARPU_MPI_LOG_IN()             do { if (!getenv("STARPU_SILENT")) { \
                                               if (_debug_rank == -1) MPI_Comm_rank(MPI_COMM_WORLD, &_debug_rank);                        \
                                               fprintf(stderr, "%*s[%d][starpu_mpi][%s:%d] -->\n", (_debug_rank+1)*4, "", _debug_rank, __starpu_func__ , __LINE__); \
                                               fflush(stderr); }} while(0)
#  define _STARPU_MPI_LOG_OUT()            do { if (!getenv("STARPU_SILENT")) { \
                                               if (_debug_rank == -1) MPI_Comm_rank(MPI_COMM_WORLD, &_debug_rank);                        \
                                               fprintf(stderr, "%*s[%d][starpu_mpi][%s:%d] <--\n", (_debug_rank+1)*4, "", _debug_rank, __starpu_func__, __LINE__ ); \
                                               fflush(stderr); }} while(0)
#else
#  define _STARPU_MPI_LOG_IN()
#  define _STARPU_MPI_LOG_OUT()
#endif

extern int _starpu_mpi_tag;

enum _starpu_mpi_request_type
{
	SEND_REQ=0,
	RECV_REQ=1,
	WAIT_REQ=2,
	TEST_REQ=3,
	BARRIER_REQ=4,
	PROBE_REQ=5,
	UNKNOWN_REQ=6,
};

struct _starpu_mpi_envelope
{
	starpu_ssize_t size;
	int data_tag;
};

struct _starpu_mpi_req;

LIST_TYPE(_starpu_mpi_req,
	/* description of the data at StarPU level */
	starpu_data_handle_t data_handle;

	/* description of the data to be sent/received */
	MPI_Datatype datatype;
	void *ptr;
	starpu_ssize_t count;
	int user_datatype;

	/* who are we talking to ? */
	int srcdst;
	int data_tag;
	MPI_Comm comm;

	void (*func)(struct _starpu_mpi_req *);

	MPI_Status *status;
	MPI_Request request;
	int *flag;
	unsigned sync;

	int ret;
	starpu_pthread_mutex_t req_mutex;
	starpu_pthread_cond_t req_cond;

	starpu_pthread_mutex_t posted_mutex;
	starpu_pthread_cond_t posted_cond;

	enum _starpu_mpi_request_type request_type; /* 0 send, 1 recv */

	unsigned submitted;
	unsigned completed;
	unsigned posted;

	UT_hash_handle hh;

	/* In the case of a Wait/Test request, we are going to post a request
	 * to test the completion of another request */
	struct _starpu_mpi_req *other_request;

	/* in the case of detached requests */
	unsigned detached;
	void *callback_arg;
	void (*callback)(void *);

        /* in the case of user-defined datatypes, we need to send the size of the data */
	MPI_Request size_req;

        struct _starpu_mpi_envelope* envelope;

	int is_internal_req;
	struct _starpu_mpi_req *internal_req;

	int sequential_consistency;
);

#ifdef __cplusplus
}
#endif

#endif // __STARPU_MPI_PRIVATE_H__
