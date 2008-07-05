#include "driver_cuda.h"

/* the number of CUDA devices */
int ncudagpus;

static CUdevice cuDevice;
static CUcontext cuContext[MAXCUDADEVS];
CUresult status;

extern char *execpath;

void init_cuda_module(struct cuda_module_s *module, char *path)
{
	unsigned i;
	for (i = 0; i < MAXCUDADEVS; i++)
	{
		module->is_loaded[i] = 0;
	}

	module->module_path = path;
}

void load_cuda_module(int devid, struct cuda_module_s *module)
{
	CUresult res;
	if (!module->is_loaded[devid])
	{
		res = cuModuleLoad(&module->module, module->module_path);
		if (res) {
			CUDA_REPORT_ERROR(res);
		}
	
		module->is_loaded[devid] = 1;
	}
}

void init_cuda_function(struct cuda_function_s *func, 
			struct cuda_module_s *module,
			char *symbol)
{
	unsigned i;
	for (i = 0; i < MAXCUDADEVS; i++)
	{
		func->is_loaded[i] = 0;
	}

	func->symbol = symbol;
	func->module = module;
}

static int testfoo = 123456;

void set_function_args(cuda_codelet_t *args, 
			buffer_descr *descr, 
			unsigned nbuffers)
{
	unsigned offset = 0;

	unsigned buf;
	for (buf = 0; buf < nbuffers; buf++)
	{
		cuParamSetv(args->func->function, offset, 
			(CUdeviceptr *)&descr[buf].ptr, sizeof(CUdeviceptr));
		offset += sizeof(CUdeviceptr);

		cuParamSetv(args->func->function, offset, 
			&descr[buf].nx, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		cuParamSetv(args->func->function, offset, 
			&descr[buf].ny, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		cuParamSetv(args->func->function, offset, 
			&descr[buf].ld, sizeof(uint32_t));
		offset += sizeof(uint32_t);
	}

	cuParamSetv(args->func->function, offset, 
		args->stack, args->stack_size);
	offset += args->stack_size;

	cuParamSetSize(args->func->function, offset);

	unsigned shmsize = args->shmemsize;
	cuFuncSetSharedSize(args->func->function, shmsize);
}

void load_cuda_function(int devid, struct cuda_function_s *function)
{
	CUresult res;

	/* load the module on the device if it is not already the case */
	load_cuda_module(devid, function->module);

	/* load the function on the device if it is not present yet */
	res = cuModuleGetFunction( &function->function, 
			function->module->module, function->symbol );
	if (res) {
		CUDA_REPORT_ERROR(res);
	}

}

void init_context(int devid)
{
	status = cuCtxCreate( &cuContext[devid], 0, 0);
	if (status) {
		CUDA_REPORT_ERROR(status);
	}

	status = cuCtxAttach(&cuContext[devid], 0);
	if (status) {
		CUDA_REPORT_ERROR(status);
	}

	cublasInit();
}

void init_cuda(void)
{
	CUresult status;

	status = cuInit(0);
	if (status) {
		CUDA_REPORT_ERROR(status);
	}

	cuDeviceGetCount(&ncudagpus);
	assert(ncudagpus <= MAXCUDADEVS);

	int dev;
	for (dev = 0; dev < ncudagpus; dev++)
	{
		// TODO change this to the driver API
		// cudaGetDeviceProperties(&cudadevprops[dev], dev);
	}
}

int execute_job_on_cuda(job_t j, int devid, unsigned use_cublas)
{
	int res;

	switch (j->type) {
		case CODELET:
			ASSERT(j);
			ASSERT(j->cl);
			fetch_codelet_input(j->buffers, j->nbuffers);

			TRACE_START_CODELET_BODY(j);
			if (use_cublas) {
				cl_func func = j->cl->cublas_func;
				ASSERT(func);
				func(j->buffers, j->cl->cl_arg);
				cuCtxSynchronize();
			} else {
				/* load the module and the function */
				cuda_codelet_t *args; 
				args = j->cl->cuda_func;

				load_cuda_function(devid, args->func);

				cuFuncSetBlockShape(args->func->function,
							args->blockx, 
							args->blocky, 1);


				/* set up the function args */
				set_function_args(args, j->buffers, j->nbuffers);

				/* set up the grids */
				cuLaunchGrid(args->func->function, 
						args->gridx, args->gridy);

				/* launch the function */
				cuCtxSynchronize();
			}
			TRACE_END_CODELET_BODY(j);	

			push_codelet_output(j->buffers, j->nbuffers, 1<<0);
			break;
		case ABORT:
			printf("CUDA abort\n");
			cublasShutdown();
			thread_exit(NULL);
			break;
		default:
			break;
	}

	return OK;
}

void *cuda_worker(void *arg)
{
	struct cuda_worker_arg_t* args = (struct cuda_worker_arg_t*)arg;

	int devid = args->deviceid;

	TRACE_NEW_WORKER(FUT_CUDA_KEY);

#ifndef DONTBIND
        /* fix the thread on the correct cpu */
        cpu_set_t aff_mask;
        CPU_ZERO(&aff_mask);
        CPU_SET(args->bindid, &aff_mask);
        sched_setaffinity(0, sizeof(aff_mask), &aff_mask);
#endif

	set_local_memory_node_key(&(((cuda_worker_arg *)arg)->memory_node));

	init_context(devid);
	fprintf(stderr, "cuda thread is ready to run on CPU %d !\n", args->bindid);

	//precondition_cuda(args->A, args->B, args->C);

	/* tell the main thread that this one is ready */
	args->ready_flag = 1;

	job_t j;
	int res;
	
	do {
		j = pop_task();
		if (j == NULL) continue;

		/* can CUDA do that task ? */
		if (!CUDA_MAY_PERFORM(j) && !CUBLAS_MAY_PERFORM(j))
		{
			/* this is neither a cuda or a cublas task */
			push_task(j);
			continue;
		}

		unsigned use_cublas = CUBLAS_MAY_PERFORM(j) ? 1:0;
		res = execute_job_on_cuda(j, devid, use_cublas);

		if (res != OK) {
			switch (res) {
				case OK:
				case FATAL:
					assert(0);
				case TRYAGAIN:
					push_task(j);
					continue;
				default:
					assert(0);
			}
		}

		if (j->cb)
			j->cb(j->argcb);
		
                /* in case there are dependencies, wake up the proper tasks */
                notify_dependencies(j);

		job_delete(j);

	} while(1);

	return NULL;

error:
	CUDA_REPORT_ERROR(status);
	assert(0);

}
