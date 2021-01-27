#!/usr/bin/bash
start=`date +%s`
export STARPU_PERF_MODEL_DIR=/usr/local/share/starpu/perfmodels/sampling
ulimit -S -s 50000000

#~ N=$((40))
#~ STARPU_SCHED=HFP ORDER_U=0 PRINTF=1 STARPU_LIMIT_BANDWIDTH=350 STARPU_TASK_PROGRESS=1 STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_CUDA_MEM=500 STARPU_DIDUSE_BARRIER=1 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/cholesky/cholesky_implicit -size $((960*N)) -nblocks $((N))

STARPU_SCHED=HFP PRINTF=2 STARPU_TASK_PROGRESS=0 BELADY=0 ORDER_U=0 STARPU_NTASKS_THRESHOLD=10 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 STARPU_DIDUSE_BARRIER=1 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*30)) -nblocks 30 -iter 1

STARPU_SCHED=HFP PRINTF=2 STARPU_TASK_PROGRESS=0 BELADY=0 ORDER_U=0 STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 STARPU_DIDUSE_BARRIER=1 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*30)) -nblocks 30 -iter 1

end=`date +%s`
runtime=$((end-start))
echo "Fin du script, l'execution a durée" $((runtime/60))" min "$((runtime%60))" sec."

#~ 2: 15 CM500 : 564
#~ 4: 15 CM500 : 564
#~ 2: 15 CM100 : 277.9
#~ 4: 15 CM100 : 305
#~ 482 / 
#~ 487 / 561
#~ Pour cholesky avec threshold: juste très long mais ca devrait passer
	
