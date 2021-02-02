start=`date +%s`
sudo make -j4
export STARPU_PERF_MODEL_DIR=/usr/local/share/starpu/perfmodels/sampling
ulimit -S -s 5000000
NB_ALGO_TESTE=8
NB_TAILLE_TESTE=10
ECHELLE_X=50
START_X=0
FICHIER=GF_M_MC_LRU_N=15_BW350_CM500_RANDOMTASKS
FICHIER_RAW=Output_maxime/GFlops_raw_out_1.txt
DOSSIER=Random_tasks
truncate -s 0 ${FICHIER_RAW:0}
echo "########## Random ##########"
for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do 
	M=$((START_X+i*ECHELLE_X))
	STARPU_SCHED=random_order STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=$M RANDOM_TASK_ORDER=0 STARPU_NCPU=0 RANDOM_DATA_ACCESS=1 SEED=1 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*15)) -nblocks 15 -iter 1 | tail -n 1 >> ${FICHIER_RAW:0}
done
echo "########## Dmdar ##########"
for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do 
	M=$((START_X+i*ECHELLE_X))
	STARPU_SCHED=dmdar STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=$M STARPU_NCPU=0 RANDOM_DATA_ACCESS=1 SEED=1 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*15)) -nblocks 15 -iter 1 | tail -n 1 >> ${FICHIER_RAW:0}
done
echo "########## HFP ##########"
for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do 
	M=$((START_X+i*ECHELLE_X))
	STARPU_SCHED=HFP STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 ORDER_U=0 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=$M STARPU_NCPU=0 RANDOM_DATA_ACCESS=1 SEED=1 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*15)) -nblocks 15 -iter 1 | tail -n 1 >> ${FICHIER_RAW:0}
done
echo "########## HFP U ##########"
for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do 
	M=$((START_X+i*ECHELLE_X))
	STARPU_SCHED=HFP STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 ORDER_U=1 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=$M STARPU_NCPU=0 RANDOM_DATA_ACCESS=1 SEED=1 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*15)) -nblocks 15 -iter 1 | tail -n 1 >> ${FICHIER_RAW:0}
done
echo "########## MST ##########"
for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do 
	M=$((START_X+i*ECHELLE_X))
	STARPU_SCHED=mst STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=$M STARPU_NCPU=0 RANDOM_DATA_ACCESS=1 SEED=1 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*15)) -nblocks 15 -iter 1 | tail -n 1 >> ${FICHIER_RAW:0}
done
echo "########## CM ##########"
for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do 
	M=$((START_X+i*ECHELLE_X))
	STARPU_SCHED=cuthillmckee STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=$M STARPU_NCPU=0 RANDOM_DATA_ACCESS=1 SEED=1 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*15)) -nblocks 15 -iter 1 | tail -n 1 >> ${FICHIER_RAW:0}
done
echo "########## HFP BELADY ##########"
for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do 
	M=$((START_X+i*ECHELLE_X))
	STARPU_SCHED=HFP STARPU_NTASKS_THRESHOLD=30 PRINTF=1 STARPU_CUDA_PIPELINE=4 BELADY=1 ORDER_U=0 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=$M STARPU_NCPU=0 RANDOM_DATA_ACCESS=1 SEED=1 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*15)) -nblocks 15 -iter 1 | tail -n 1 >> ${FICHIER_RAW:0}
done
echo "########## HFP U BELADY ##########"
for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do 
	M=$((START_X+i*ECHELLE_X))
	STARPU_SCHED=HFP STARPU_NTASKS_THRESHOLD=30 STARPU_CUDA_PIPELINE=4 BELADY=1 ORDER_U=1 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=$M STARPU_NCPU=0 RANDOM_DATA_ACCESS=1 SEED=1 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -xy $((960*15)) -nblocks 15 -iter 1 | tail -n 1 >> ${FICHIER_RAW:0}
done
gcc -o cut_gflops_raw_out cut_gflops_raw_out.c
./cut_gflops_raw_out $NB_TAILLE_TESTE $NB_ALGO_TESTE $ECHELLE_X $START_X ${FICHIER_RAW:0} ../these_gonthier_maxime/Starpu/R/Data/${DOSSIER}/${FICHIER:0}.txt
Rscript /home/gonthier/these_gonthier_maxime/Starpu/R/ScriptR/${DOSSIER}/${FICHIER:0}.R
mv /home/gonthier/starpu/Rplots.pdf /home/gonthier/these_gonthier_maxime/Starpu/R/Courbes/${DOSSIER}/${FICHIER:0}.pdf
end=`date +%s`
runtime=$((end-start))
echo "Fin du script, l'execution a durée" $((runtime/60))" min "$((runtime%60))" sec."
