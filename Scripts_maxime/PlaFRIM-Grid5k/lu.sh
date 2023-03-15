# bash Scripts_maxime/PlaFRIM-Grid5k/lu.sh NGPU TAILLE_TUILE NB_TAILLE_TESTE MEMOIRE MODEL
# oarsub -t exotic -p "network_address in ('gemini-1.lyon.grid5000.fr')" -r '2023-02-17 11:51:00' -l walltime=04:00:00 "bash Scripts_maxime/PlaFRIM-Grid5k/cholesky_dependances.sh 1 1920 12 2000 opti"

# bash Scripts_maxime/PlaFRIM-Grid5k/lu.sh 1 1920 7 2000 best_ones
# bash Scripts_maxime/PlaFRIM-Grid5k/lu.sh 2 1920 7 2000 best_ones
# bash Scripts_maxime/PlaFRIM-Grid5k/lu.sh 4 1920 7 2000 best_ones
# bash Scripts_maxime/PlaFRIM-Grid5k/lu.sh 8 1920 7 2000 best_ones

if [ $# != 5 ]
then
	echo "Arguments must be: bash Scripts_maxime/lu.sh NGPU TAILLE_TUILE NB_TAILLE_TESTE MEMOIRE MODEL"
	exit
fi

make -j 6
START_X=0
ECHELLE_X=6
FICHIER_RAW=Output_maxime/GFlops_raw_out_1.txt
FICHIER_BUS=Output_maxime/GFlops_raw_out_2.txt
FICHIER_RAW_DT=Output_maxime/GFlops_raw_out_3.txt
ulimit -S -s 50000000
truncate -s 0 ${FICHIER_RAW}
truncate -s 0 ${FICHIER_RAW_DT}
truncate -s 0 ${FICHIER_BUS}

NGPU=$1

TH=10
CP=5

SEED=1

NITER=1

TAILLE_TUILE=$2

CM=$4
MODEL=$5

echo "CM =" ${CM} "BLOCK SIZE =" ${TAILLE_TUILE} "NGPU =" ${NGPU} ${MODEL}

NCOMBINAISONS=4
if [ NGPU != 1 ]
then
	NCOMBINAISONS=$((NGPU*2+(NGPU-1)*NGPU+3))
fi

# Best results is obtained with following parameters so far:
# EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1
# THRESHOLD=0ou2
# APP=1
# CHOOSE_BEST_DATA_FROM=0 (need to test with 1 in real)
# SIMULATE_MEMORY=0 (need to test with 1 in real)
# TASK_ORDER=1-2
# DATA_ORDER=1-2
# STARPU_SCHED_READY=1
# DEPENDANCES=1
# PRIO=1
# FREE_PUSHED_TASK_POSITION=1
# DOPT_SELECTION_ORDER=1 if I don't consider transfer time, else it's 4 or 7 for the ratio
# PRIORITY_ATTRIBUTION=1or3 (1 is bottom level, 3 is PaRSEC)
# GRAPH_DESCENDANTS=0
# HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1
# CAN_A_DATA_BE_IN_MEM_AND_IN_NOT_USED_YET=0
# PUSH_FREE_TASK_ON_GPU_WITH_LEAST_TASK_IN_PLANNED_TASK=0ou1ou2 0and2 takes less time so there's that. 2 is more fair

NB_TAILLE_TESTE=$3

PRIORITY_ATTRIBUTION=1

if [ ${MODEL} == "best_ones" ]; then
	NB_ALGO_TESTE=6
	# Best ones
	algo1="DMDAR"
	algo2="DMDAS"
	algo3="LWS"
	algo4="DARTS"
	algo5="DARTS"
	
	if [ $((CM)) == 0 ]; then
		echo "#### eager ####"
		for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do
			N=$((START_X+i*ECHELLE_X))
			echo "N=${N}"
			PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) SEED=$((N/5)) STARPU_SCHED=modular-eager-prefetching STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_EXPECTED_TRANSFER_TIME_WRITEBACK=1 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/lu/lu_example_double -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
			sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
		done
		echo "#### ${algo1} ####"
		for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do
			N=$((START_X+i*ECHELLE_X))
			echo "N=${N}"
			PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) SEED=$((N/5)) STARPU_SCHED=dmdar STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_EXPECTED_TRANSFER_TIME_WRITEBACK=1 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/lu/lu_example_double -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
			sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
		done
		echo "#### ${algo2} ####"
		for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do
			N=$((START_X+i*ECHELLE_X))
			echo "N=${N}"
			PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) SEED=$((N/5)) STARPU_SCHED=dmdas STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_EXPECTED_TRANSFER_TIME_WRITEBACK=1 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/lu/lu_example_double -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
			sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
		done
		echo "#### ${algo3} ####"
		for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do
			N=$((START_X+i*ECHELLE_X))
			echo "N=${N}"
			PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) SEED=$((N/5)) STARPU_SCHED=lws STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_EXPECTED_TRANSFER_TIME_WRITEBACK=1 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/lu/lu_example_double -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
			sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
		done
		echo "#### 4 ####"
		for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do
			N=$((START_X+i*ECHELLE_X))
			echo "N=${N}"
			THRESHOLD=0 DOPT_SELECTION_ORDER=4 CHOOSE_BEST_DATA_FROM=0 SIMULATE_MEMORY=0 CAN_A_DATA_BE_IN_MEM_AND_IN_NOT_USED_YET=0 PRIORITY_ATTRIBUTION=1 HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 PUSH_FREE_TASK_ON_GPU_WITH_LEAST_TASK_IN_PLANNED_TASK=2 STARPU_SCHED_READY=1 TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/lu/lu_example_double -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
			sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
		done
		echo "#### 5 ####"
		for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do
			N=$((START_X+i*ECHELLE_X))
			echo "N=${N}"
			THRESHOLD=0 DOPT_SELECTION_ORDER=7 CHOOSE_BEST_DATA_FROM=0 SIMULATE_MEMORY=0 CAN_A_DATA_BE_IN_MEM_AND_IN_NOT_USED_YET=0 PRIORITY_ATTRIBUTION=1 HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 PUSH_FREE_TASK_ON_GPU_WITH_LEAST_TASK_IN_PLANNED_TASK=2 STARPU_SCHED_READY=1 TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/lu/lu_example_double -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
			sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
		done
	else
		echo "#### eager ####"
		for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do
			N=$((START_X+i*ECHELLE_X))
			echo "N=${N}"
			PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) STARPU_LIMIT_CUDA_MEM=$((CM)) SEED=$((N/5)) STARPU_SCHED=modular-eager-prefetching STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_EXPECTED_TRANSFER_TIME_WRITEBACK=1 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/lu/lu_example_double -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
			sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
		done
		echo "#### ${algo1} ####"
		for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do
			N=$((START_X+i*ECHELLE_X))
			echo "N=${N}"
			PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) STARPU_LIMIT_CUDA_MEM=$((CM)) SEED=$((N/5)) STARPU_SCHED=dmdar STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_EXPECTED_TRANSFER_TIME_WRITEBACK=1 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/lu/lu_example_double -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
			sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
		done
		echo "#### ${algo2} ####"
		for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do
			N=$((START_X+i*ECHELLE_X))
			echo "N=${N}"
			PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) STARPU_LIMIT_CUDA_MEM=$((CM)) SEED=$((N/5)) STARPU_SCHED=dmdas STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_EXPECTED_TRANSFER_TIME_WRITEBACK=1 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/lu/lu_example_double -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
			sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
		done
		echo "#### ${algo3} ####"
		for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do
			N=$((START_X+i*ECHELLE_X))
			echo "N=${N}"
			PRIORITY_ATTRIBUTION=$((PRIORITY_ATTRIBUTION)) STARPU_LIMIT_CUDA_MEM=$((CM)) SEED=$((N/5)) STARPU_SCHED=lws STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_EXPECTED_TRANSFER_TIME_WRITEBACK=1 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/lu/lu_example_double -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
			sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
		done
		echo "#### 4 ####"
		for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do
			N=$((START_X+i*ECHELLE_X))
			echo "N=${N}"
			THRESHOLD=0 DOPT_SELECTION_ORDER=4 CHOOSE_BEST_DATA_FROM=0 SIMULATE_MEMORY=0 CAN_A_DATA_BE_IN_MEM_AND_IN_NOT_USED_YET=0 PRIORITY_ATTRIBUTION=1 STARPU_LIMIT_CUDA_MEM=$((CM)) HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 PUSH_FREE_TASK_ON_GPU_WITH_LEAST_TASK_IN_PLANNED_TASK=2 STARPU_SCHED_READY=1 TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/lu/lu_example_double -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
			sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
		done
		echo "#### 5 ####"
		for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
		do
			N=$((START_X+i*ECHELLE_X))
			echo "N=${N}"
			THRESHOLD=0 DOPT_SELECTION_ORDER=7 CHOOSE_BEST_DATA_FROM=0 SIMULATE_MEMORY=0 CAN_A_DATA_BE_IN_MEM_AND_IN_NOT_USED_YET=0 PRIORITY_ATTRIBUTION=1 STARPU_LIMIT_CUDA_MEM=$((CM)) HIGHEST_PRIORITY_TASK_RETURNED_IN_DEFAULT_CASE=1 GRAPH_DESCENDANTS=0 PUSH_FREE_TASK_ON_GPU_WITH_LEAST_TASK_IN_PLANNED_TASK=2 STARPU_SCHED_READY=1 TASK_ORDER=2 DATA_ORDER=2 FREE_PUSHED_TASK_POSITION=1 DEPENDANCES=1 PRIO=1 APP=1 SEED=$((N/5)) EVICTION_STRATEGY_DYNAMIC_DATA_AWARE=1 STARPU_SCHED=dynamic-data-aware STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS}" STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 ./examples/lu/lu_example_double -size $((TAILLE_TUILE*N)) -nblocks $((N)) | tail -n 1 >> ${FICHIER_RAW}
			sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS} >> ${FICHIER_RAW_DT}
		done
	fi
fi

if [ $((CM)) == 0 ]; then
	CM=32000
fi

echo "Converting data"
mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/LU/GF_${MODEL}_${TAILLE_TUILE}_${NGPU}GPU_${CM}Mo.csv

mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/LU/DT_${MODEL}_${TAILLE_TUILE}_${NGPU}GPU_${CM}Mo.csv
