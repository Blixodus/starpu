#!/usr/bin/bash
start=`date +%s`
#~ sudo make -j4
#~ sudo make -j4 -C src/
#~ sudo make -j4 -C examples/
export STARPU_PERF_MODEL_DIR=/usr/local/share/starpu/perfmodels/sampling
ulimit -S -s 50000000

#~ i=5 ; STARPU_BUS_STATS=1 STARPU_SCHED=HFP ORDER_U=1 PRINTF=2 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_TASK_PROGRESS=1 BELADY=1 STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 STARPU_GENERATE_TRACE=0 STARPU_LIMIT_CUDA_MEM=500 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/cholesky/cholesky_implicit -size $((960*i)) -nblocks $((i))

#~ i= ; STARPU_SCHED=dmdar STARPU_TASK_PROGRESS=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=350 STARPU_GENERATE_TRACE=0 STARPU_LIMIT_CUDA_MEM=500 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -3d -xy $((960*i)) -nblocks $((i)) -nblocksz 4 -iter 1

#~ ./examples/cholesky/cholesky_implicit -size $((960*i)) -nblocks $((i))

#~ truncate -s 0 Output_maxime/GFlops_raw_out_3.txt
#~ i=5 ;  STARPU_SCHED=HFP PRINTF=0 STARPU_TASK_PROGRESS=0 BELADY=1 STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="Output_maxime/BUS_STATS.txt" ORDER_U=1 STARPU_CUDA_PIPELINE=4 STARPU_NTASKS_THRESHOLD=30 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*$i)) -nblocks $i -iter 1
#~ sed -n '4p' Output_maxime/BUS_STATS.txt > Output_maxime/GFlops_raw_out_3.txt

#~ i=15 ; STARPU_BUS_STATS=1 STARPU_SCHED=dmdar PRINTF=2 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_TASK_PROGRESS=1 BELADY=0 ORDER_U=0 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -3d -xy $((960*i)) -nblocks $((i)) -nblocksz 4 -iter 1

i=5 ; PRINTF=1 STARPU_GENERATE_TRACE=0 STARPU_BUS_STATS=0 STARPU_NTASKS_THRESHOLD=30 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_CUDA_PIPELINE=4 STARPU_SCHED=mst PRINTF=0 STARPU_TASK_PROGRESS=1 BELADY=0 ORDER_U=1 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=100 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -3d -xy $((960*i)) -nblocks $((i)) -nblocksz 4 -iter 1

#~ i=15 ; STARPU_SCHED=HFP PRINTF=1 STARPU_BUS_STATS=1 STARPU_TASK_PROGRESS=0 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_GENERATE_TRACE=0 STARPU_SIMGRID_CUDA_MALLOC_COST=0 BELADY=0 ORDER_U=1 STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=200 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*$i)) -nblocks $i -iter 1

#~ M3D N15 HFPUB : 354
#~ M3D N20 HFPUB : 369.2
#~ M2D N15	HFPUB :


#~ 10: 140.8 26 / 147 102 / 332 68
#~ 30:	139.7 26 / 143 102 / 323 72

#~ NB_TAILLE_TESTE=10
#~ ECHELLE_X=50
#~ START_X=0
#~ echo "########## HFP ##########"
#~ for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	#~ do 
	#~ M=$((START_X+i*ECHELLE_X))
	#~ STARPU_SCHED=HFP BELADY=1 STARPU_NTASKS_THRESHOLD=30 ORDER_U=0 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=$M STARPU_DIDUSE_BARRIER=1 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*15)) -nblocks 15 -iter 1 | tail 
#~ done
#~ echo "########## HFP U ##########"
#~ for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	#~ do 
	#~ M=$((START_X+i*ECHELLE_X))
	#~ STARPU_SCHED=HFP BELADY=1 STARPU_NTASKS_THRESHOLD=30 ORDER_U=1 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=$M STARPU_DIDUSE_BARRIER=1 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*15)) -nblocks 15 -iter 1
#~ done

#~ ECHELLE_X=5
#~ NB_TAILLE_TESTE=10
#~ echo "########## HFP U THRESHOLD ##########"
#~ for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	#~ do 
	#~ T=$((i*ECHELLE_X))
	#~ STARPU_SCHED=HFP BELADY=1 ORDER_U=1 STARPU_NTASKS_THRESHOLD=$T STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=200 STARPU_DIDUSE_BARRIER=1 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*20)) -nblocks 20 -iter 1 | tail -n 1 >> Output_maxime/GFlops_raw_1.txt
#~ done

#~ START_X=0
#~ NB_TAILLE_TESTE=10
#~ ECHELLE_X=50
#~ echo "########## HFP BELADY ##########"
#~ for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	#~ do 
	#~ M=$((START_X+i*ECHELLE_X))
	#~ STARPU_SCHED=HFP BELADY=1 STARPU_NTASKS_THRESHOLD=30 ORDER_U=0 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=$M STARPU_DIDUSE_BARRIER=1 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -3d -xy $((960*15)) -nblocks $((15)) -nblocksz 4 -iter 1
#~ done
#~ echo "########## HFP U BELADY ##########"
#~ for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	#~ do 
	#~ M=$((START_X+i*ECHELLE_X))
	#~ STARPU_SCHED=HFP BELADY=1 STARPU_NTASKS_THRESHOLD=30 ORDER_U=1 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=$M STARPU_DIDUSE_BARRIER=1 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -3d -xy $((960*15)) -nblocks $((15)) -nblocksz 4 -iter 1
#~ done

end=`date +%s`
runtime=$((end-start))
echo "Fin du script, l'execution a durée" $((runtime/60))" min "$((runtime%60))" sec."

#~ C = 960*2*N
#~ A,B = 960*4*960*N
