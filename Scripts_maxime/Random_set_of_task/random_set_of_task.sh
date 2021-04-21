#!/usr/bin/bash
#bash Scripts_maxime/Random_set_of_task/random_set_of_task.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 2 Random_set_of_task number_task_1gpu
#bash Scripts_maxime/Random_set_of_task/random_set_of_task.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 2 Random_set_of_task number_data_1gpu
#bash Scripts_maxime/Random_set_of_task/random_set_of_task.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 2 Random_set_of_task degree_max_1gpu

#~ ./examples/random_task_graph/random_task_graph -ntasks 10 -ndata 10 -degreemax 5

PATH_STARPU=$1
PATH_R=$2
NB_TAILLE_TESTE=$3
DOSSIER=$4
MODEL=$5
NB_ALGO_TESTE=7
FICHIER_RAW=${PATH_STARPU}/starpu/Output_maxime/GFlops_raw_out_1.txt
export STARPU_PERF_MODEL_DIR=/usr/local/share/starpu/perfmodels/sampling
ulimit -S -s 5000000
truncate -s 0 ${FICHIER_RAW}
if [ $MODEL = number_task_1gpu ]
then
	ECHELLE_X=100
	START_X=0  
	DEGREEMAX=20
	NDATA=100
	echo "############## Modular random prio prefetching ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		NTASK=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=modular-random-prio-prefetching STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
	echo "############## Modular eager prefetching ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		NTASK=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=modular-eager-prefetching STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
	echo "############## Dmdar ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		NTASK=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=dmdar STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
		echo "############## MST ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		NTASK=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=mst STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
	echo "############## HFP U TH30 ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		NTASK=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=HFP STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 ORDER_U=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
	echo "############## HFP U TH0 ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		NTASK=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=HFP STARPU_NTASKS_THRESHOLD=0 STARPU_CUDA_PIPELINE=4 ORDER_U=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
	echo "############## Modular heft idle th30 + HFP ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		NTASK=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=modular-heft-HFP MODULAR_HEFT_HFP_MODE=2 SEED=1 STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 ORDER_U=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
fi
if [ $MODEL = number_data_1gpu ]
then
	ECHELLE_X=20
	START_X=0  
	DEGREEMAX=20
	NTASK=500
	echo "############## Modular random prio prefetching ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		NDATA=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=modular-random-prio-prefetching STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
	echo "############## Modular eager prefetching ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		NDATA=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=modular-eager-prefetching STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
	echo "############## Dmdar ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		NDATA=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=dmdar STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
		echo "############## MST ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		NDATA=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=mst STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
	echo "############## HFP U TH30 ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		NDATA=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=HFP STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 ORDER_U=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
	echo "############## HFP U TH0 ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		NDATA=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=HFP STARPU_NTASKS_THRESHOLD=0 STARPU_CUDA_PIPELINE=4 ORDER_U=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
	echo "############## Modular heft idle th30 + HFP ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		NDATA=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=modular-heft-HFP MODULAR_HEFT_HFP_MODE=2 SEED=1 STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 ORDER_U=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
fi
if [ $MODEL = degree_max_1gpu ]
then
	ECHELLE_X=2
	START_X=0  
	NDATA=100
	NTASK=500
	echo "############## Modular random prio prefetching ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		DEGREEMAX=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=modular-random-prio-prefetching STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
	echo "############## Modular eager prefetching ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		DEGREEMAX=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=modular-eager-prefetching STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
	echo "############## Dmdar ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		DEGREEMAX=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=dmdar STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
		echo "############## MST ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		DEGREEMAX=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=mst STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
	echo "############## HFP U TH30 ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		DEGREEMAX=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=HFP STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 ORDER_U=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
	echo "############## HFP U TH0 ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		DEGREEMAX=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=HFP STARPU_NTASKS_THRESHOLD=0 STARPU_CUDA_PIPELINE=4 ORDER_U=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=350 SEED=1 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
	echo "############## Modular heft idle th30 + HFP ##############"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do 
		DEGREEMAX=$((START_X+i*ECHELLE_X))
		STARPU_SCHED=modular-heft-HFP MODULAR_HEFT_HFP_MODE=2 SEED=1 STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 ORDER_U=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/random_task_graph/random_task_graph -ntasks $((NTASK)) -ndata $((NDATA)) -degreemax $((DEGREEMAX)) | tail -n 1 >> ${FICHIER_RAW}
	done
fi

# Tracage des GFlops
gcc -o cut_gflops_raw_out cut_gflops_raw_out.c
./cut_gflops_raw_out $NB_TAILLE_TESTE $NB_ALGO_TESTE $ECHELLE_X $START_X ${FICHIER_RAW} ${PATH_R}/R/Data/${DOSSIER}/GF_${MODEL}.txt
Rscript ${PATH_R}/R/ScriptR/GF_X.R ${PATH_R}/R/Data/${DOSSIER}/GF_${MODEL}.txt ${MODEL} ${DOSSIER}
mv ${PATH_STARPU}/starpu/Rplots.pdf ${PATH_R}/R/Courbes/${DOSSIER}/GF_${MODEL}.pdf

