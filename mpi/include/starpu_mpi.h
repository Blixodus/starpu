/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2009-2012, 2014  Université de Bordeaux
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

#ifndef __STARPU_MPI_H__
#define __STARPU_MPI_H__

#include <starpu.h>

#if defined(STARPU_USE_MPI)

#include <mpi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *starpu_mpi_req;

int starpu_mpi_isend(starpu_data_handle_t data_handle, starpu_mpi_req *req, int dest, int mpi_tag, MPI_Comm comm);
int starpu_mpi_irecv(starpu_data_handle_t data_handle, starpu_mpi_req *req, int source, int mpi_tag, MPI_Comm comm);
int starpu_mpi_send(starpu_data_handle_t data_handle, int dest, int mpi_tag, MPI_Comm comm);
int starpu_mpi_recv(starpu_data_handle_t data_handle, int source, int mpi_tag, MPI_Comm comm, MPI_Status *status);
int starpu_mpi_isend_detached(starpu_data_handle_t data_handle, int dest, int mpi_tag, MPI_Comm comm, void (*callback)(void *), void *arg);
int starpu_mpi_irecv_detached(starpu_data_handle_t data_handle, int source, int mpi_tag, MPI_Comm comm, void (*callback)(void *), void *arg);
int starpu_mpi_issend(starpu_data_handle_t data_handle, starpu_mpi_req *req, int dest, int mpi_tag, MPI_Comm comm);
int starpu_mpi_wait(starpu_mpi_req *req, MPI_Status *status);
int starpu_mpi_test(starpu_mpi_req *req, int *flag, MPI_Status *status);
int starpu_mpi_barrier(MPI_Comm comm);

int starpu_mpi_irecv_detached_sequential_consistency(starpu_data_handle_t data_handle, int source, int mpi_tag, MPI_Comm comm, void (*callback)(void *), void *arg, int sequential_consistency);

int starpu_mpi_init(int *argc, char ***argv, int initialize_mpi);
int starpu_mpi_initialize(void) STARPU_DEPRECATED;
int starpu_mpi_initialize_extended(int *rank, int *world_size) STARPU_DEPRECATED;
int starpu_mpi_shutdown(void);

struct starpu_task *starpu_mpi_task_build(MPI_Comm comm, struct starpu_codelet *codelet, ...);
int starpu_mpi_task_post_build(MPI_Comm comm, struct starpu_codelet *codelet, ...);
int starpu_mpi_task_insert(MPI_Comm comm, struct starpu_codelet *codelet, ...);
/* the function starpu_mpi_insert_task has the same semantics as starpu_mpi_task_insert, it is kept to avoid breaking old codes */
int starpu_mpi_insert_task(MPI_Comm comm, struct starpu_codelet *codelet, ...);

void starpu_mpi_get_data_on_node(MPI_Comm comm, starpu_data_handle_t data_handle, int node);
void starpu_mpi_get_data_on_node_detached(MPI_Comm comm, starpu_data_handle_t data_handle, int node, void (*callback)(void*), void *arg);
void starpu_mpi_redux_data(MPI_Comm comm, starpu_data_handle_t data_handle);

int starpu_mpi_scatter_detached(starpu_data_handle_t *data_handles, int count, int root, MPI_Comm comm, void (*scallback)(void *), void *sarg, void (*rcallback)(void *), void *rarg);
int starpu_mpi_gather_detached(starpu_data_handle_t *data_handles, int count, int root, MPI_Comm comm, void (*scallback)(void *), void *sarg, void (*rcallback)(void *), void *rarg);

int starpu_mpi_isend_detached_unlock_tag(starpu_data_handle_t data_handle, int dest, int mpi_tag, MPI_Comm comm, starpu_tag_t tag);
int starpu_mpi_irecv_detached_unlock_tag(starpu_data_handle_t data_handle, int source, int mpi_tag, MPI_Comm comm, starpu_tag_t tag);

int starpu_mpi_isend_array_detached_unlock_tag(unsigned array_size, starpu_data_handle_t *data_handle, int *dest, int *mpi_tag, MPI_Comm *comm, starpu_tag_t tag);
int starpu_mpi_irecv_array_detached_unlock_tag(unsigned array_size, starpu_data_handle_t *data_handle, int *source, int *mpi_tag, MPI_Comm *comm, starpu_tag_t tag);

void starpu_mpi_comm_amounts_retrieve(size_t *comm_amounts);

void starpu_mpi_cache_flush(MPI_Comm comm, starpu_data_handle_t data_handle);
void starpu_mpi_cache_flush_all_data(MPI_Comm comm);

int starpu_mpi_world_rank(void);

int starpu_mpi_get_communication_tag(void);
void starpu_mpi_set_communication_tag(int tag);

void starpu_mpi_data_register(starpu_data_handle_t data_handle, int tag, int rank);

char *starpu_mpi_node_selection_get_default_policy();
int starpu_mpi_node_selection_set_default_policy(char *policy);

#ifdef __cplusplus
}
#endif

#endif // STARPU_USE_MPI
#endif // __STARPU_MPI_H__
