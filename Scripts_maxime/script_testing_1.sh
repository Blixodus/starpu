#!/usr/bin/bash
start=`date +%s`
export STARPU_PERF_MODEL_DIR=/usr/local/share/starpu/perfmodels/sampling
ulimit -S -s 50000000

#~ i=15 ; STARPU_NTASKS_THRESHOLD=30 PRINTF=1 STARPU_CUDA_PIPELINE=4 STARPU_SCHED=HFP STARPU_TASK_PROGRESS=0 BELADY=0 ORDER_U=1 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -3d -xy $((960*i)) -nblocks $((i)) -nblocksz $((4)) -iter 1

#~ i=50 ; STARPU_NTASKS_THRESHOLD=30 PRINTF=1 STARPU_CUDA_PIPELINE=4 STARPU_SCHED=mst STARPU_TASK_PROGRESS=1 BELADY=0 ORDER_U=0 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*i)) -nblocks $((i)) -iter 1

#~ i=30 ; BELADY=1 PRINTF=1 STARPU_SCHED=mst ORDER_U=0 STARPU_NTASKS_THRESHOLD=30 STARPU_TASK_PROGRESS=1 STARPU_CUDA_PIPELINE=4 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/cholesky/cholesky_implicit -size $((960*i)) -nblocks $((i))

#### Multi GPU #### STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0
#~ i=18 ; STARPU_NTASKS_THRESHOLD=30 MODULAR_HEFT_HFP_MODE=2 MULTIGPU=4 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_GENERATE_TRACE=0 PRINTF=1 STARPU_CUDA_PIPELINE=4 STARPU_SCHED=modular-heft-HFP STARPU_SIMGRID_CUDA_MALLOC_COST=0 ORDER_U=1 STARPU_BUS_STATS=0 STARPU_LIMIT_BANDWIDTH=1050 STARPU_LIMIT_CUDA_MEM=250 STARPU_NCPU=0 STARPU_NCUDA=3 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*i)) -nblocks $((i)) -iter 1

#~ echo "3 10 1 1 1 1 0 0" > Output_maxime/hMETIS_parameters.txt 
i=10 ; STARPU_NTASKS_THRESHOLD=30 HMETIS=1 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_GENERATE_TRACE=0 PRINTF=0 STARPU_CUDA_PIPELINE=4 STARPU_SCHED=HFP STARPU_SIMGRID_CUDA_MALLOC_COST=0 ORDER_U=1 STARPU_WORKER_STATS=1 STARPU_LIMIT_BANDWIDTH=1050 STARPU_LIMIT_CUDA_MEM=250 STARPU_NCPU=0 STARPU_NCUDA=3 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*i)) -nblocks $((i)) -iter 1

#~ N=30 ; STARPU_SCHED=HFP READY=1 STARPU_NTASKS_THRESHOLD=0 STARPU_GENERATE_TRACE=1 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_CUDA_PIPELINE=4 BELADY=0 ORDER_U=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*N)) -nblocks $((N)) -iter 1
#~ N=25 ; STARPU_SCHED=HFP STARPU_NTASKS_THRESHOLD=30 PRINTF=1 STARPU_CUDA_PIPELINE=4 ORDER_U=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 RANDOM_TASK_ORDER=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*N)) -nblocks $((N)) -iter 1
#~ i=35 ; STARPU_NTASKS_THRESHOLD=30 PRINTF=1 STARPU_WORKER_STATS=1 STARPU_BUS_STATS=0 MULTIGPU=0 STARPU_GENERATE_TRACE=0 STARPU_CUDA_PIPELINE=4 STARPU_SCHED=HFP STARPU_TASK_PROGRESS=0 STARPU_GENERATE_TRACE=0 BELADY=0 ORDER_U=1 STARPU_LIMIT_BANDWIDTH=1050 STARPU_LIMIT_CUDA_MEM=250 STARPU_NCPU=0 STARPU_NCUDA=3 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/cholesky/cholesky_implicit -size $((960*i)) -nblocks $((i))
#~ i=2 ; STARPU_NTASKS_THRESHOLD=30 PRINTF=1 STARPU_WORKER_STATS=0 STARPU_BUS_STATS=0 MULTIGPU=0 STARPU_GENERATE_TRACE=0 STARPU_CUDA_PIPELINE=4 STARPU_SCHED=HFP STARPU_TASK_PROGRESS=0 STARPU_GENERATE_TRACE=1 BELADY=0 ORDER_U=1 STARPU_LIMIT_BANDWIDTH=1050 STARPU_LIMIT_CUDA_MEM=250 STARPU_NCPU=0 STARPU_NCUDA=3 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -3d -xy $((960*i)) -nblocks $((i)) -nblocksz $((4)) -iter 1
#~ i=20 ; STARPU_NTASKS_THRESHOLD=30 PRINTF=1 MULTIGPU=2 STARPU_GENERATE_TRACE=0 STARPU_CUDA_PIPELINE=4 STARPU_SCHED=HFP STARPU_TASK_PROGRESS=0 BELADY=0 ORDER_U=1 STARPU_LIMIT_BANDWIDTH=1050 STARPU_LIMIT_CUDA_MEM=500 STARPU_NCPU=0 STARPU_NCUDA=3 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*i)) -nblocks $((i)) -iter 1
#~ i=20 ; STARPU_NTASKS_THRESHOLD=30 PRINTF=1 MULTIGPU=3 STARPU_GENERATE_TRACE=0 STARPU_CUDA_PIPELINE=4 STARPU_SCHED=HFP STARPU_TASK_PROGRESS=0 BELADY=0 ORDER_U=1 STARPU_LIMIT_BANDWIDTH=1050 STARPU_LIMIT_CUDA_MEM=500 STARPU_NCPU=0 STARPU_NCUDA=3 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*i)) -nblocks $((i)) -iter 1


#~ i=44 ;
#~ STARPU_SCHED=dmdar STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/cholesky/cholesky_implicit -size $((960*i)) -nblocks $((i)) -no-prio

#~ STARPU_SCHED=modular-heft STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_SCHED_SORTED_ABOVE=0 STARPU_SCHED_SORTED_BELOW=0 STARPU_NTASKS_THRESHOLD=0 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/cholesky/cholesky_implicit -size $((960*i)) -nblocks $((i)) -no-prio


#~ libtool --mode=execute gdb --args

end=`date +%s`
runtime=$((end-start))
echo "Fin du script, l'execution a durée" $((runtime/60))" min "$((runtime%60))" sec."
