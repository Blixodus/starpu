/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2011, 2012, 2013, 2014  Centre National de la Recherche Scientifique
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

#include <starpu_mpi.h>
#include <math.h>

#define FPRINTF(ofile, fmt, ...) do { if (!getenv("STARPU_SSILENT")) {fprintf(ofile, fmt, ## __VA_ARGS__); }} while(0)

void stencil5_cpu(void *descr[], STARPU_ATTRIBUTE_UNUSED void *_args)
{
	unsigned *xy = (unsigned *)STARPU_VARIABLE_GET_PTR(descr[0]);
	unsigned *xm1y = (unsigned *)STARPU_VARIABLE_GET_PTR(descr[1]);
	unsigned *xp1y = (unsigned *)STARPU_VARIABLE_GET_PTR(descr[2]);
	unsigned *xym1 = (unsigned *)STARPU_VARIABLE_GET_PTR(descr[3]);
	unsigned *xyp1 = (unsigned *)STARPU_VARIABLE_GET_PTR(descr[4]);

	//FPRINTF(stdout, "VALUES: %d %d %d %d %d\n", *xy, *xm1y, *xp1y, *xym1, *xyp1);
	*xy = (*xy + *xm1y + *xp1y + *xym1 + *xyp1) / 5;
}

struct starpu_codelet stencil5_cl =
{
	.cpu_funcs = {stencil5_cpu, NULL},
	.nbuffers = 5,
	.modes = {STARPU_RW, STARPU_R, STARPU_R, STARPU_R, STARPU_R}
};

#ifdef STARPU_QUICK_CHECK
#  define NITER_DEF	5
#  define X         	3
#  define Y         	3
#else
#  define NITER_DEF	500
#  define X         	20
#  define Y         	20
#endif

int display = 0;
int niter = NITER_DEF;

/* Returns the MPI node number where data indexes index is */
int my_distrib(int x, int y, int nb_nodes)
{
	/* Block distrib */
	return ((int)(x / sqrt(nb_nodes) + (y / sqrt(nb_nodes)) * sqrt(nb_nodes))) % nb_nodes;
}

/* Shifted distribution, for migration example */
int my_distrib2(int x, int y, int nb_nodes)
{
	return (my_distrib(x, y, nb_nodes) + 1) % nb_nodes;
}


static void parse_args(int argc, char **argv)
{
	int i;
	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-iter") == 0)
		{
			char *argptr;
			niter = strtol(argv[++i], &argptr, 10);
		}
		if (strcmp(argv[i], "-display") == 0)
		{
			display = 1;
		}
	}
}

int main(int argc, char **argv)
{
	int my_rank, size, x, y, loop;
	int value=0, mean=0;
	unsigned matrix[X][Y];
	starpu_data_handle_t data_handles[X][Y];

	int ret = starpu_init(NULL);
	STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");
	starpu_mpi_init(&argc, &argv, 1);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	parse_args(argc, argv);

	/* Initial data values */
	for(x = 0; x < X; x++)
	{
		for (y = 0; y < Y; y++)
		{
			matrix[x][y] = (my_rank+1)*10 + value;
			value++;
			mean += matrix[x][y];
		}
	}
	mean /= value;

	/* Initial distribution */
	for(x = 0; x < X; x++)
	{
		for (y = 0; y < Y; y++)
		{
			int mpi_rank = my_distrib(x, y, size);
			if (mpi_rank == my_rank)
			{
				//FPRINTF(stderr, "[%d] Owning data[%d][%d]\n", my_rank, x, y);
				starpu_variable_data_register(&data_handles[x][y], STARPU_MAIN_RAM, (uintptr_t)&(matrix[x][y]), sizeof(unsigned));
			}
			else if (my_rank == my_distrib(x+1, y, size) || my_rank == my_distrib(x-1, y, size)
				 || my_rank == my_distrib(x, y+1, size) || my_rank == my_distrib(x, y-1, size))
			{
				/* I don't own that index, but will need it for my computations */
				//FPRINTF(stderr, "[%d] Neighbour of data[%d][%d]\n", my_rank, x, y);
				starpu_variable_data_register(&data_handles[x][y], -1, (uintptr_t)NULL, sizeof(unsigned));
			}
			else
			{
				/* I know it's useless to allocate anything for this */
				data_handles[x][y] = NULL;
			}
			if (data_handles[x][y])
			{
				starpu_mpi_data_register(data_handles[x][y], (y*X)+x, mpi_rank);
			}
		}
	}

	/* First computation with initial distribution */
	for(loop=0 ; loop<niter; loop++)
	{
		for (x = 1; x < X-1; x++)
		{
			for (y = 1; y < Y-1; y++)
			{
				starpu_mpi_task_insert(MPI_COMM_WORLD, &stencil5_cl, STARPU_RW, data_handles[x][y],
						       STARPU_R, data_handles[x-1][y], STARPU_R, data_handles[x+1][y],
						       STARPU_R, data_handles[x][y-1], STARPU_R, data_handles[x][y+1],
						       0);
			}
		}
	}
	FPRINTF(stderr, "Waiting ...\n");
	starpu_task_wait_for_all();

	/* Now migrate data to a new distribution */

	/* First register newly needed data */
	for(x = 0; x < X; x++)
	{
		for (y = 0; y < Y; y++)
		{
			int mpi_rank = my_distrib2(x, y, size);
			if (!data_handles[x][y] && (mpi_rank == my_rank
				 || my_rank == my_distrib2(x+1, y, size) || my_rank == my_distrib2(x-1, y, size)
				 || my_rank == my_distrib2(x, y+1, size) || my_rank == my_distrib2(x, y-1, size)))
			{
				/* Register newly-needed data */
				starpu_variable_data_register(&data_handles[x][y], -1, (uintptr_t)NULL, sizeof(unsigned));
				starpu_mpi_data_register(data_handles[x][y], (y*X)+x, mpi_rank);
			}
			if (data_handles[x][y] && mpi_rank != starpu_data_get_rank(data_handles[x][y]))
			{
				/* Migrate the data */
				starpu_mpi_get_data_on_node_detached(MPI_COMM_WORLD, data_handles[x][y], mpi_rank, NULL, NULL);
				/* And register new rank of the matrix */
				starpu_data_set_rank(data_handles[x][y], mpi_rank);
			}
		}
	}

	/* Second computation with new distribution */
	for(loop=0 ; loop<niter; loop++)
	{
		for (x = 1; x < X-1; x++)
		{
			for (y = 1; y < Y-1; y++)
			{
				starpu_mpi_task_insert(MPI_COMM_WORLD, &stencil5_cl, STARPU_RW, data_handles[x][y],
						       STARPU_R, data_handles[x-1][y], STARPU_R, data_handles[x+1][y],
						       STARPU_R, data_handles[x][y-1], STARPU_R, data_handles[x][y+1],
						       0);
			}
		}
	}
	FPRINTF(stderr, "Waiting ...\n");
	starpu_task_wait_for_all();

	/* Unregister data */
	for(x = 0; x < X; x++)
	{
		for (y = 0; y < Y; y++)
		{
			if (data_handles[x][y])
			{
				int mpi_rank = my_distrib(x, y, size);
				/* Get back data to original place where the user-provided buffer is. */
				starpu_mpi_get_data_on_node_detached(MPI_COMM_WORLD, data_handles[x][y], mpi_rank, NULL, NULL);
				/* Register original rank of the matrix (although useless) */
				starpu_data_set_rank(data_handles[x][y], mpi_rank);
				/* And unregister it */
				starpu_data_unregister(data_handles[x][y]);
			}
		}
	}

	starpu_mpi_shutdown();
	starpu_shutdown();

	if (display)
	{
		FPRINTF(stdout, "[%d] mean=%d\n", my_rank, mean);
		for(x = 0; x < X; x++)
		{
			FPRINTF(stdout, "[%d] ", my_rank);
			for (y = 0; y < Y; y++)
			{
				FPRINTF(stdout, "%3u ", matrix[x][y]);
			}
			FPRINTF(stdout, "\n");
		}
	}

	return 0;
}
