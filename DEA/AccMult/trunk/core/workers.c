#include <common/timing.h>
#include <common/fxt.h>
#include "workers.h"
#include "jobs.h"

/* number of actual CPU cores */

#ifdef USE_CPUS
unsigned ncores;
thread_t corethreads[NMAXCORES];
core_worker_arg coreargs[NMAXCORES];
#endif

#ifdef USE_CUDA
thread_t cudathreads[MAXCUDADEVS];
cuda_worker_arg cudaargs[MAXCUDADEVS];
extern int ncudagpus;
#endif

#ifdef USE_CUBLAS
thread_t cublasthreads[MAXCUBLASDEVS];
cublas_worker_arg cublasargs[MAXCUBLASDEVS];
unsigned ncublasgpus;
#endif

#ifdef USE_SPU
thread_t sputhreads[MAXSPUS];
unsigned nspus;
spu_worker_arg spuargs[MAXSPUS];
#endif

#ifdef USE_GORDON
thread_t gordonthread;
/* only the threads managed by gordon */
unsigned ngordonspus;
gordon_worker_arg gordonargs;
#endif

int current_bindid = 0;

void init_machine(void)
{
	int envval;

	srand(2008);

#ifdef USE_FXT
	start_fxt_profiling();
#endif

#ifdef USE_CPUS
	envval = get_env_number("NCPUS");
	if (envval < 0) {
		ncores = MIN(sysconf(_SC_NPROCESSORS_ONLN), NMAXCORES);
	} else {
		/* use the specified value */
		ncores = (unsigned)envval;
		ASSERT(ncores <= NMAXCORES);
	}
#endif

#ifdef USE_CUDA
	envval = get_env_number("NCUDA");
	if (envval < 0) {
//		ncudagpus = MIN(get_cuda_device_count(), MAXCUDADEVS);
		init_cuda();
	} else {
		/* use the specified value */
		ncudagpus = (unsigned)envval;
		ASSERT(ncudagpus <= MAXCUDADEVS);

		if (ncudagpus > 0)
			init_cuda();
	}

#endif

#ifdef USE_CUBLAS
	envval = get_env_number("NCUBLAS");
	if (envval < 0) {
		ncublasgpus = MIN(get_cublas_device_count(), MAXCUBLASDEVS);
	} else {
		/* use the specified value */
		ncublasgpus = (unsigned)envval;
		ASSERT(ncublasgpus <= MAXCUBLASDEVS);
	}
#endif

#ifdef USE_SPU
	envval = get_env_number("NSPUS");
	if (envval < 0) {
		nspus = MIN(get_spu_count(), MAXSPUS);
	} else {
		/* use the specified value */
		nspus = (unsigned)envval;
		ASSERT(nspus <= MAXSPUS);
	}

#endif

	/* for the data wizard */
	init_memory_nodes();

	timing_init();
}

void init_workers(void)
{
	/* initialize the queue containing the jobs */
	init_work_queue();

	/* launch one thread per CPU */
	unsigned memory_node;

	/* note that even if the CPU core are not used, we always have a RAM node */
	/* TODO : support NUMA  ;) */
	memory_node = register_memory_node(RAM);

#ifdef USE_CPUS
	unsigned core;
	for (core = 0; core < ncores; core++)
	{
		coreargs[core].bindid =
			(current_bindid++) % (sysconf(_SC_NPROCESSORS_ONLN));

		coreargs[core].coreid = core;
		coreargs[core].ready_flag = 0;

		coreargs[core].memory_node = memory_node;

		thread_create(&corethreads[core], NULL, core_worker, &coreargs[core]);
		/* wait until the thread is actually launched ... */
		while (coreargs[core].ready_flag == 0) {}
	}
#endif

#ifdef USE_CUDA
	/* initialize CUDA with the proper number of threads */
	int cudadev;
	for (cudadev = 0; cudadev < ncudagpus; cudadev++)
	{
		cudaargs[cudadev].deviceid = cudadev;
		cudaargs[cudadev].ready_flag = 0;

		cudaargs[cudadev].bindid =
			(current_bindid++) % (sysconf(_SC_NPROCESSORS_ONLN));

		cudaargs[cudadev].memory_node =
			register_memory_node(CUDA_RAM);

		thread_create(&cudathreads[cudadev], NULL, cuda_worker,
				(void*)&cudaargs[cudadev]);

		/* wait until the thread is actually launched ... */
		while (cudaargs[cudadev].ready_flag == 0) {}
	}
#endif

#ifdef USE_CUBLAS
	/* initialize CUBLAS with the proper number of threads */
	unsigned cublasdev;
	for (cublasdev = 0; cublasdev < ncublasgpus; cublasdev++)
	{
		cublasargs[cublasdev].deviceid = cublasdev;
		cublasargs[cublasdev].ready_flag = 0;

		cublasargs[cublasdev].bindid =
			(current_bindid++) % (sysconf(_SC_NPROCESSORS_ONLN));

		cublasargs[cublasdev].memory_node =
			register_memory_node(CUBLAS_RAM);

		thread_create(&cublasthreads[cublasdev], NULL, cublas_worker,
			(void*)&cublasargs[cublasdev]);

		/* wait until the thread is actually launched ... */
		while (cublasargs[cublasdev].ready_flag == 0) {}
	}
#endif

#ifdef USE_SPU
	/* initialize the various SPUs  */
	unsigned spu;
	for (spu = 0; spu < nspus; spu++)
	{
		spuargs[spu].deviceid = spu;
		spuargs[spu].ready_flag = 0;

		spuargs[spu].bindid =
			(current_bindid++) % (sysconf(_SC_NPROCESSORS_ONLN));

		spuargs[spu].memory_node =
			register_memory_node(SPU_LS);

		thread_create(&sputhreads[spu], NULL, spu_worker,
			(void*)&spuargs[spu]);

		/* wait until the thread is actually launched ... */
		while (spuargs[spu].ready_flag == 0) {}
	}
#endif

#ifdef USE_GORDON
	ngordonspus = 8;
	gordonargs.ready_flag = 0;

	gordonargs.bindid = (current_bindid++) % (sysconf(_SC_NPROCESSORS_ONLN));
	gordonargs.nspus = ngordonspus;

	/* do not forget to registrate memory nodes for each SPUs later on ! */

	thread_create(&gordonthread, NULL, gordon_worker, (void*)&gordonargs);

	/* wait until the thread is actually launched ... */
	while (gordonargs.ready_flag == 0) {}
#endif
}

void terminate_workers(void)
{
	fprintf(stderr, "terminate workers \n");
#ifdef USE_CPUS
	unsigned core;
	for (core = 0; core < ncores; core++)
	{
		thread_join(corethreads[core], NULL);
	}
	fprintf(stderr, "core terminated ... \n");
#endif

#ifdef USE_CUDA
	int cudadev;
	for (cudadev = 0; cudadev < ncudagpus; cudadev++)
	{
		thread_join(cudathreads[cudadev], NULL);
	}
	fprintf(stderr, "cuda terminated\n");
#endif

#ifdef USE_CUBLAS
	unsigned cublasdev;
	for (cublasdev = 0; cublasdev < ncublasgpus; cublasdev++)
	{
		thread_join(cublasthreads[cublasdev], NULL);
	}
	fprintf(stderr, "cublas terminated\n");
#endif

#ifdef USE_SPU
	unsigned spu;
	for (spu = 0; spu < nspus; spu++)
	{
		thread_join(sputhreads[spu], NULL);
	}
	fprintf(stderr, "SPUs terminated\n");
#endif


}

void kill_all_workers(void)
{
        /* terminate all threads */
        unsigned nworkers = 0;

#ifdef USE_CPUS
        nworkers += ncores;
#endif
#ifdef USE_CUDA
        nworkers += ncudagpus;
#endif
#ifdef USE_CUBLAS
        nworkers += ncublasgpus;
#endif
#ifdef USE_SPU
        nworkers += nspus;
#endif

        unsigned worker;
        for (worker = 0; worker < nworkers ; worker++) {
                job_t j = job_new();
                j->type = ABORT;
                j->where = ANY;
                push_task(j);
        }

        if (nworkers == 0) {
                fprintf(stderr, "Warning there is no worker ... \n");
        }

}

void fetch_codelet_input(buffer_descr *descrs, data_interface_t *interface, unsigned nbuffers)
{
	TRACE_START_FETCH_INPUT(NULL);

	/* TODO we should avoid repeatingly ask for the local thread index etc. */
	unsigned index;
	for (index = 0; index < nbuffers; index++)
	{
		buffer_descr *descr;
		uint32_t local_memory_node = get_local_memory_node();

		descr = &descrs[index];

		fetch_data(descr->state, descr->mode);

		descr->interfaceid = descr->state->interfaceid;

		memcpy(&interface[index], &descr->state->interface[local_memory_node], 
				sizeof(data_interface_t));
	}

	TRACE_END_FETCH_INPUT(NULL);
}

void push_codelet_output(buffer_descr *descrs, unsigned nbuffers, uint32_t mask)
{
	TRACE_START_PUSH_OUTPUT(NULL);

	unsigned index;
	for (index = 0; index < nbuffers; index++)
	{
		release_data(descrs[index].state, mask);
	}

	TRACE_END_PUSH_OUTPUT(NULL);
}
