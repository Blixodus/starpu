#!/usr/bin/bash
#~ bash Scripts_maxime/script_testing_1.sh

start=`date +%s`
#~ export STARPU_PERF_MODEL_DIR=/usr/local/share/starpu/perfmodels/sampling
make -C src/ -j 6
#~ make -j 6

#~ libtool --mode=execute strace ./examples/mult/sgemm

#~ libtool --mode=execute strace -o /tmp/log ./examples/mult/sgemm

#~ srun --exclusive -C sirocco21 --pty bash Scripts_maxime/task_stealing.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 2 Matrice_ligne task_stealing -i

#~ bash Scripts_maxime/task_stealing.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 2 Matrice_ligne task_stealing

#~ ./examples/mult/sgemm -3d -xy $((960*N)) -nblocks $((N)) -nblocksz $((4)) -iter 1
#~ ./examples/mult/sgemm -3d -xyz $((960*N)) -nblocks $((N)) -nblocksz $((N)) -iter 1
#~ ./examples/mult/sgemm -xy $((960*N)) -nblocks $((N)) -iter 1
#~ ./examples/cholesky/cholesky_implicit -size $((960*N)) -nblocks $((N))
#~ ./examples/random_task_graph/random_task_graph -ntasks 10 -ndata 10 -degreemax 5

#~ libtool --mode=execute gdb --args 
#~ r backtrace l p

#~ HOST="attila"
#~ ORDO="dynamic-outer"
#~ N=7
#~ CUDAMEM=500
#~ NGPU=1
#~ BW=350

#~ SEED=1 STARPU_SCHED=${ORDO} EVICTION_STRATEGY_DYNAMIC_OUTER=0 DATA_POP_POLICY=1 PRINTF=1 PRINT_N=$((N)) STARPU_GENERATE_TRACE=0 STARPU_SCHED_READY=0 STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=30 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=$((BW)) STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_LIMIT_CUDA_MEM=$((CUDAMEM)) STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*N)) -nblocks $((N)) -iter 1 

#~ python3 /home/gonthier/these_gonthier_maxime/Code/visualisation2D.py Output_maxime/Data_coordinates_order_last_SCHEDULER.txt Output_maxime/Data_to_load_SCHEDULER.txt ${N} ${ORDO} ${NGPU} 1

#~ watch *(int *)0x555555876ab0 task 0x555555b20da0

#~ watch *(int *)0x5555558771c0
#~ c
#~ c
#~ disable b 1

#~ watch *(int *)0x5555558c2820
#~ watch *(int *)0x5555558c2820

N=10
NGPU=1
ORDO="dynamic-outer"
BW=350
CM=250
EVICTION=1
POP_POLICY=1
READY=0
export STARPU_PERF_MODEL_DIR=tools/perfmodels/sampling
ulimit -S -s 5000000

#~ ORDO="HFP"
#~ STARPU_SCHED=${ORDO} SEED=0 EVICTION_STRATEGY_DYNAMIC_OUTER=$((EVICTION)) DATA_POP_POLICY=$((POP_POLICY)) STARPU_GENERATE_TRACE=0 PRINTF=1 PRINT_N=$((N)) PRINT3D=1 STARPU_SCHED_READY=$((READY)) STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=30 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=$((BW)) STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_LIMIT_CUDA_MEM=$((CM)) STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -3d -xy $((960*N)) -nblocks $((N)) -nblocksz $((4)) -iter 1
#~ python3 /home/gonthier/these_gonthier_maxime/Code/visualisation2D.py Output_maxime/Data_coordinates_order_last_SCHEDULER.txt Output_maxime/Data_to_load_SCHEDULER.txt ${N} ${ORDO} ${NGPU} 4

STARPU_SCHED=${ORDO} SEED=0 EVICTION_STRATEGY_DYNAMIC_OUTER=$((EVICTION)) DATA_POP_POLICY=$((POP_POLICY)) STARPU_GENERATE_TRACE=0 PRINTF=1 PRINT_N=$((N)) STARPU_SCHED_READY=$((READY)) STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=30 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=$((BW)) STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_LIMIT_CUDA_MEM=$((CM)) STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*N)) -nblocks $((N)) -iter 1 2>&1 | tee Output_maxime/terminal_output.txt

#~ python3 /home/gonthier/these_gonthier_maxime/Code/visualisation2D.py Output_maxime/Data_coordinates_order_last_SCHEDULER.txt Output_maxime/Data_to_load_SCHEDULER.txt ${N} ${ORDO} ${NGPU} 1

#~ setarch linux64 -R libtool --mode=execute gdb --args

end=`date +%s` runtime=$((end-start)) echo "Fin du script, l'execution a durée" $((runtime/60))" min "$((runtime%60))" sec."
