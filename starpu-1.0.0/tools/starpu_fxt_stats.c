/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2011  Centre National de la Recherche Scientifique
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

//#include "fxt_tool.h"

#include <search.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <common/fxt.h>
#include <common/list.h>
#include <starpu.h>


static fxt_t fut;
struct fxt_ev_64 ev;

static uint64_t transfers[16][16]; 

static void handle_data_copy(void)
{
	unsigned src = ev.param[0];
	unsigned dst = ev.param[1];
	unsigned size = ev.param[2];

	transfers[src][dst] += size;

//	printf("transfer %d -> %d : %d \n", src, dst, size);
}

/*
 * This program should be used to parse the log generated by FxT 
 */
int main(int argc, char **argv)
{
	char *filename = NULL;
	int ret;
	int fd_in; 

	if (argc < 2) {
	        fprintf(stderr, "Usage : %s input_filename [-o output_filename]\n", argv[0]);
	        exit(-1);
	}
	
	filename = argv[1];
	

	fd_in = open(filename, O_RDONLY);
	if (fd_in < 0) {
	        perror("open failed :");
	        exit(-1);
	}

	fut = fxt_fdopen(fd_in);
	if (!fut) {
	        perror("fxt_fdopen :");
	        exit(-1);
	}
	
	fxt_blockev_t block;
	block = fxt_blockev_enter(fut);

	unsigned njob = 0;
	unsigned nws = 0;

	double start_time = 10e30;
	double end_time = -10e30;

	while(1) {
		ret = fxt_next_ev(block, FXT_EV_TYPE_64, (struct fxt_ev *)&ev);
		if (ret != FXT_EV_OK) {
			fprintf(stderr, "no more block ...\n");
			break;
		}

		end_time = STARPU_MAX(end_time, ev.time);
		start_time = STARPU_MIN(start_time, ev.time);

		__attribute__ ((unused)) int nbparam = ev.nb_params;

		switch (ev.code) {
			case _STARPU_FUT_DATA_COPY:
				handle_data_copy();
				break;
			case _STARPU_FUT_JOB_POP:
				njob++;
				break;
			case _STARPU_FUT_WORK_STEALING:
				nws++;
				break;
			default:
				break;
		}
	}

	printf("Start : start time %e end time %e length %e\n", start_time, end_time, end_time - start_time);

	unsigned src, dst;
	for (src = 0; src < 16; src++)
	{
		for (dst = 0; dst < 16; dst++)
		{
			if (transfers[src][dst] != 0) {
				printf("%d -> %d \t %ld MB\n", src, dst, transfers[src][dst]/(1024*1024));
			}
		}
	}

	printf("There was %d tasks and %d work stealing\n", njob, nws);

	return 0;
}
