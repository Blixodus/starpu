# bash Scripts_maxime/quick_plot1.sh NGPU TAILLE_TUILE NB_TAILLE_TESTE MEMOIRE ${MODEL}

# bash Scripts_maxime/quick_plot1.sh 1 960 12 500 ${MODEL}
# bash Scripts_maxime/quick_plot1.sh 1 1920 12 2000 ${MODEL}
# bash Scripts_maxime/quick_plot1.sh 1 3840 12 8000 ${MODEL}

if [ $# != 5  ]
then
	echo "Arguments must be: bash Scripts_maxime/quick_plot1.sh NGPU TAILLE_TUILE NB_TAILLE_TESTE MEMOIRE MODEL"
	exit
fi

make -j 6
export STARPU_PERF_MODEL_DIR=tools/perfmodels/sampling

START_X=0
ECHELLE_X=5
FICHIER_RAW=/home/gonthier/starpu/Output_maxime/GFlops_raw_out_1.txt
FICHIER_BUS=/home/gonthier/starpu/Output_maxime/GFlops_raw_out_2.txt
FICHIER_RAW_DT=/home/gonthier/starpu/Output_maxime/GFlops_raw_out_3.txt
ulimit -S -s 50000000
truncate -s 0 ${FICHIER_RAW}
truncate -s 0 ${FICHIER_RAW_DT}
truncate -s 0 ${FICHIER_BUS}

NGPU=$1

#~ CM=500
#~ CM=0 # 0 = infinie
#~ CM=100

TH=10

CP=5

#~ HOST="gemini-1-fgcs-36" # cop coll multi gpus
#~ HOST="gemini-1-cho_dep" # from grid5k perfs, multiple sizes
HOST="gemini-1-cho_dep_corrected" # from grid5k perfs, multiple sizes corrected links

SEED=1

NITER=1

TAILLE_TUILE=$2

CM=$4
MODEL=$5

echo "CM =" ${CM} "BLOCK SIZE =" ${TAILLE_TUILE} "HOST =" ${HOST} "NGPU =" ${NGPU} "MODEL =" ${MODEL}

NCOMBINAISONS=4
if [ NGPU != 1 ]
then
	NCOMBINAISONS=$((NGPU*2+(NGPU-1)*NGPU+3))
fi

# Best results is obtained with following parameters so far:
# EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1
# THRESHOLD=0
# APP=1
# CHOOSE_BEST_DATA_FROM=0 (need to test with 1 in real)
# SIMULATE_MEMORY=0 (need to test with 1 in real)
# TASK_ORDER=1-2
# DATA_ORDER=1-2
# STARPU_SCHED_READY=1
# DEPENDANCES=1
# PRIO=1
# FREE_PUSHED_TASK_POSITION=1
# DOPT_SELECTION_ORDER=1 if I don't consider transfer time, else it's 4 if it works as intented
# PRIORITY_ATTRIBUTION=1 (1 is bottom level)
# GRAPH_DESCENDANTS=0
# HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1
# CAN_A_DATA_BE_IN_MEM_AND_IN_NOT_USED_YET=0
# PUSH_FREE_TASK_ON_GPU_WITH_LEAST_TASK_IN_PLANNED_TASK=0ou1 0 takes less time so there's that

# PM = push middle
# PT = push top
# R = ready
# NTO = natural task order
# NDO = natural data order
# 3D = 1 from free task considered
# LUF = Eviction policy
# BL = priority bottom level
# BLT = priority bottom level with times
# DSL = data selection locality
# DSP = data selection priority
# GD0 = natural prio in tasks
# GD1 = graph descendants prio in tasks with a pause on all tasks
# GD2 = graph descendants prio in tasks without pause and update in pull task
# HPT = highest priority task returned in default case

# Best ones
if [ ${MODEL} == "best_ones" ]; then
	NB_ALGO_TESTE=4
elif [ ${MODEL} == "TransferTimeOrder" ]; then
	NB_ALGO_TESTE=6
elif [ ${MODEL} == "DataInMemAndNotUsedYet" ] || [ ${MODEL} == "GpuChoiceFreeTask" ]; then
	NB_ALGO_TESTE=2
else
	echo "Modèle pas géré"
	exit
fi

NB_TAILLE_TESTE=$3

# Prios
#~ echo "#### ${algo1} ####"
#~ for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
#~ do
	#~ N=$((START_X+i*ECHELLE_X)#~ done

#~ echo "#### ${algo4} ####"
#~ for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
#~ do
	#~ N=$((START_X+i*ECHELLE_X))#~ done

#~ echo "#### ${algo2} ####"
#~ for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
#~ do
	#~ N=$((START_X+i*ECHELLE_X))
#~ do
	#~ N=$((START_X+i*ECHELLE_X))
	#~ echo "N=${N}"
	#~ HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 DOPT_SELECTION_ORDER=1 STARPU_SCHED_READY=1 PRIORITY_ATTRIBUTION=1 TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_HOSTNAME=${HOST} STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/cholesky/cholesky_implicit -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
	#~ sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
#~ done

#~ echo "#### ${algo6} ####"
#~ for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
#~ do
	#~ N=$((START_X+i*ECHELLE_X))
	#~ echo "N=${N}"
	#~ CHOOSE_BEST_DATA_FROM=1 SIMULATE_MEMORY=1 STARPU_LIMIT_CUDA_MEM=$((CM)) HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 DOPT_SELECTION_ORDER=1 STARPU_SCHED_READY=1 PRIORITY_ATTRIBUTION=0 TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_HOSTNAME=${HOST} STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/cholesky/cholesky_implicit -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
	#~ sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
#~ done

#~ PRIORITY_ATTRIBUTION=0
PRIORITY_ATTRIBUTION=1

# Best ones
if [ ${MODEL} == "best_ones" ]; then
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do
		N=$((START_X+i*ECHELLE_X))
		echo "N=${N}"
		PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) STARPU_LIMIT_CUDA_MEM=$((CM)) STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_HOSTNAME=${HOST} SEED=$((N/5)) STARPU_SCHED=dmdar STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_EXPECTED_TRANSFER_TIME_WRITEBACK=1 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/cholesky/cholesky_implicit -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
		sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
	done
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do
		N=$((START_X+i*ECHELLE_X))
		echo "N=${N}"
		PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) STARPU_LIMIT_CUDA_MEM=$((CM)) STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_HOSTNAME=${HOST} SEED=$((N/5)) STARPU_SCHED=dmdas STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_EXPECTED_TRANSFER_TIME_WRITEBACK=1 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/cholesky/cholesky_implicit -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
		sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
	done
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do
		N=$((START_X+i*ECHELLE_X))
		echo "N=${N}"
		PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) STARPU_LIMIT_CUDA_MEM=$((CM)) STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_HOSTNAME=${HOST} SEED=$((N/5)) STARPU_SCHED=lws STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_EXPECTED_TRANSFER_TIME_WRITEBACK=1 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/cholesky/cholesky_implicit -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
		sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
	done
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do
		N=$((START_X+i*ECHELLE_X))
		echo "N=${N}"
		CHOOSE_BEST_DATA_FROM=0 SIMULATE_MEMORY=0 STARPU_LIMIT_CUDA_MEM=$((CM)) HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 DOPT_SELECTION_ORDER=1 STARPU_SCHED_READY=1 PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_HOSTNAME=${HOST} STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/cholesky/cholesky_implicit -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
		sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
	done
elif [ ${MODEL} == "DataInMemAndNotUsedYet" ]; then
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do
		N=$((START_X+i*ECHELLE_X))
		echo "N=${N}"
		CAN_A_DATA_BE_IN_MEM_AND_IN_NOT_USED_YET=1 CHOOSE_BEST_DATA_FROM=0 SIMULATE_MEMORY=0 STARPU_LIMIT_CUDA_MEM=$((CM)) HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 DOPT_SELECTION_ORDER=1 STARPU_SCHED_READY=1 PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_HOSTNAME=${HOST} STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/cholesky/cholesky_implicit -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
		sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
	done
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do
		N=$((START_X+i*ECHELLE_X))
		echo "N=${N}"
		CAN_A_DATA_BE_IN_MEM_AND_IN_NOT_USED_YET=0 CHOOSE_BEST_DATA_FROM=0 SIMULATE_MEMORY=0 STARPU_LIMIT_CUDA_MEM=$((CM)) HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 DOPT_SELECTION_ORDER=1 STARPU_SCHED_READY=1 PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_HOSTNAME=${HOST} STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/cholesky/cholesky_implicit -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
		sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
	done
elif [ ${MODEL} == "GpuChoiceFreeTask" ]; then
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do
		N=$((START_X+i*ECHELLE_X))
		echo "N=${N}"
		PUSH_FREE_TASK_ON_GPU_WITH_LEAST_TASK_IN_PLANNED_TASK=0 CAN_A_DATA_BE_IN_MEM_AND_IN_NOT_USED_YET=0 CHOOSE_BEST_DATA_FROM=0 SIMULATE_MEMORY=0 STARPU_LIMIT_CUDA_MEM=$((CM)) HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 DOPT_SELECTION_ORDER=1 STARPU_SCHED_READY=1 PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_HOSTNAME=${HOST} STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/cholesky/cholesky_implicit -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
		sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
	done
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do
		N=$((START_X+i*ECHELLE_X))
		echo "N=${N}"
		PUSH_FREE_TASK_ON_GPU_WITH_LEAST_TASK_IN_PLANNED_TASK=1 CAN_A_DATA_BE_IN_MEM_AND_IN_NOT_USED_YET=0 CHOOSE_BEST_DATA_FROM=0 SIMULATE_MEMORY=0 STARPU_LIMIT_CUDA_MEM=$((CM)) HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 DOPT_SELECTION_ORDER=1 STARPU_SCHED_READY=1 PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_HOSTNAME=${HOST} STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/cholesky/cholesky_implicit -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
		sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
	done
elif [ ${MODEL} == "TransferTimeOrder" ]; then
	echo "########## 1 ##########"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do
		N=$((START_X+i*ECHELLE_X))
		echo "N=${N}"
		PUSH_FREE_TASK_ON_GPU_WITH_LEAST_TASK_IN_PLANNED_TASK=1 CAN_A_DATA_BE_IN_MEM_AND_IN_NOT_USED_YET=0 CHOOSE_BEST_DATA_FROM=0 SIMULATE_MEMORY=0 STARPU_LIMIT_CUDA_MEM=$((CM)) HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 DOPT_SELECTION_ORDER=1 STARPU_SCHED_READY=1 PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_HOSTNAME=${HOST} STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/cholesky/cholesky_implicit -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
		sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
	done
	echo "########## 2 ##########"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do
		N=$((START_X+i*ECHELLE_X))
		echo "N=${N}"
		PUSH_FREE_TASK_ON_GPU_WITH_LEAST_TASK_IN_PLANNED_TASK=1 CAN_A_DATA_BE_IN_MEM_AND_IN_NOT_USED_YET=0 CHOOSE_BEST_DATA_FROM=0 SIMULATE_MEMORY=0 STARPU_LIMIT_CUDA_MEM=$((CM)) HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 DOPT_SELECTION_ORDER=2 STARPU_SCHED_READY=1 PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_HOSTNAME=${HOST} STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/cholesky/cholesky_implicit -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
		sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
	done
	echo "########## 3 ##########"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do
		N=$((START_X+i*ECHELLE_X))
		echo "N=${N}"
		PUSH_FREE_TASK_ON_GPU_WITH_LEAST_TASK_IN_PLANNED_TASK=1 CAN_A_DATA_BE_IN_MEM_AND_IN_NOT_USED_YET=0 CHOOSE_BEST_DATA_FROM=0 SIMULATE_MEMORY=0 STARPU_LIMIT_CUDA_MEM=$((CM)) HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 DOPT_SELECTION_ORDER=3 STARPU_SCHED_READY=1 PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_HOSTNAME=${HOST} STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/cholesky/cholesky_implicit -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
		sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
	done
	echo "########## 4 ##########"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do
		N=$((START_X+i*ECHELLE_X))
		echo "N=${N}"
		PUSH_FREE_TASK_ON_GPU_WITH_LEAST_TASK_IN_PLANNED_TASK=1 CAN_A_DATA_BE_IN_MEM_AND_IN_NOT_USED_YET=0 CHOOSE_BEST_DATA_FROM=0 SIMULATE_MEMORY=0 STARPU_LIMIT_CUDA_MEM=$((CM)) HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 DOPT_SELECTION_ORDER=4 STARPU_SCHED_READY=1 PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_HOSTNAME=${HOST} STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/cholesky/cholesky_implicit -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
		sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
	done
	echo "########## 5 ##########"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do
		N=$((START_X+i*ECHELLE_X))
		echo "N=${N}"
		PUSH_FREE_TASK_ON_GPU_WITH_LEAST_TASK_IN_PLANNED_TASK=1 CAN_A_DATA_BE_IN_MEM_AND_IN_NOT_USED_YET=0 CHOOSE_BEST_DATA_FROM=0 SIMULATE_MEMORY=0 STARPU_LIMIT_CUDA_MEM=$((CM)) HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 DOPT_SELECTION_ORDER=5 STARPU_SCHED_READY=1 PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_HOSTNAME=${HOST} STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/cholesky/cholesky_implicit -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
		sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
	done
	echo "########## 6 ##########"
	for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do
		N=$((START_X+i*ECHELLE_X))
		echo "N=${N}"
		PUSH_FREE_TASK_ON_GPU_WITH_LEAST_TASK_IN_PLANNED_TASK=1 CAN_A_DATA_BE_IN_MEM_AND_IN_NOT_USED_YET=0 CHOOSE_BEST_DATA_FROM=0 SIMULATE_MEMORY=0 STARPU_LIMIT_CUDA_MEM=$((CM)) HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 DOPT_SELECTION_ORDER=6 STARPU_SCHED_READY=1 PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SIMGRID_CUDA_MALLOC_COST=0 STARPU_HOSTNAME=${HOST} STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/cholesky/cholesky_implicit -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
		sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
	done
fi

echo "Converting data"
gcc -o cut_gflops_raw_out_csv cut_gflops_raw_out_csv.c
./cut_gflops_raw_out_csv $NB_TAILLE_TESTE $NB_ALGO_TESTE $ECHELLE_X $START_X ${FICHIER_RAW} /home/gonthier/these_gonthier_maxime/Starpu/R/Data/quick_plot.csv
gcc -o cut_datatransfers_raw_out_csv cut_datatransfers_raw_out_csv.c
./cut_datatransfers_raw_out_csv $NB_TAILLE_TESTE $NB_ALGO_TESTE $ECHELLE_X $START_X $NGPU ${FICHIER_RAW_DT} /home/gonthier/these_gonthier_maxime/Starpu/R/Data/quick_plot_dt.csv
