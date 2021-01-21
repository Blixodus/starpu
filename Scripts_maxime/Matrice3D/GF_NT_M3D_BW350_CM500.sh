#!/usr/bin/bash
start=`date +%s`
sudo make -j4
export STARPU_PERF_MODEL_DIR=/usr/local/share/starpu/perfmodels/sampling
truncate -s 0 Output_maxime/GFlops_raw_out.txt
ulimit -S -s 50000000
NB_ALGO_TESTE=6
NB_TAILLE_TESTE=4
ECHELLE_X=5
START_X=0
FICHIER=GF_NT_M3D_BW350_CM500
echo "############## Random_order ##############"
for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do 
	N=$((START_X+i*ECHELLE_X))
	STARPU_SCHED=random_order STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 RANDOM_TASK_ORDER=0 STARPU_DIDUSE_BARRIER=1 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -3d -xy $((960*N)) -z $((960*N)) -nblocks $((N)) -iter 1 | tail -n 1 >> Output_maxime/GFlops_raw_out.txt
	echo "ok"
done
echo "############## Dmdar ##############"
for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do 
	N=$((START_X+i*ECHELLE_X))
	STARPU_SCHED=dmdar STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 RANDOM_TASK_ORDER=0 STARPU_DIDUSE_BARRIER=1 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -3d -xy $((960*N)) -z $((960*N)) -nblocks $((N)) -iter 1 | tail -n 1 >> Output_maxime/GFlops_raw_out.txt
done
echo "############## HFP ##############"
for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do 
	N=$((START_X+i*ECHELLE_X))
	STARPU_SCHED=HFP STARPU_NTASKS_THRESHOLD=30 ORDER_U=0 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 RANDOM_TASK_ORDER=0 STARPU_DIDUSE_BARRIER=1 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -3d -xy $((960*N)) -z $((960*N)) -nblocks $((N)) -iter 1 | tail -n 1 >> Output_maxime/GFlops_raw_out.txt
done
echo "############## HFP U ##############"
for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do 
	N=$((START_X+i*ECHELLE_X))
	STARPU_SCHED=HFP STARPU_NTASKS_THRESHOLD=30 ORDER_U=1 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 RANDOM_TASK_ORDER=0 STARPU_DIDUSE_BARRIER=1 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -3d -xy $((960*N)) -z $((960*N)) -nblocks $((N)) -iter 1 | tail -n 1 >> Output_maxime/GFlops_raw_out.txt
done
echo "############## MST ##############"
for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do 
	N=$((START_X+i*ECHELLE_X))
	STARPU_SCHED=mst STARPU_NTASKS_THRESHOLD=30 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 STARPU_DIDUSE_BARRIER=1 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -3d -xy $((960*N)) -z $((960*N)) -nblocks $((N)) -iter 1 | tail -n 1 >> Output_maxime/GFlops_raw_out.txt
done
echo "############## CM ##############"
for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do 
	N=$((START_X+i*ECHELLE_X))
	STARPU_SCHED=cuthillmckee STARPU_NTASKS_THRESHOLD=30 STARPU_LIMIT_BANDWIDTH=350 STARPU_LIMIT_CUDA_MEM=500 STARPU_DIDUSE_BARRIER=1 STARPU_NCPU=0 STARPU_NCUDA=1 STARPU_NOPENCL=0 STARPU_HOSTNAME=attila ./examples/mult/sgemm -3d -xy $((960*N)) -z $((960*N)) -nblocks $((N)) -iter 1 | tail -n 1 >> Output_maxime/GFlops_raw_out.txt
done
gcc -o cut_gflops_raw_out cut_gflops_raw_out.c
./cut_gflops_raw_out $NB_TAILLE_TESTE $NB_ALGO_TESTE $ECHELLE_X $START_X
mv Output_maxime/GFlops_data_out.txt /home/gonthier/these_gonthier_maxime/Starpu/R/Data/Matrice3D/${FICHIER:0}.txt
Rscript /home/gonthier/these_gonthier_maxime/Starpu/R/ScriptR/Matrice3D/${FICHIER:0}.R
mv /home/gonthier/starpu/Rplots.pdf /home/gonthier/these_gonthier_maxime/Starpu/R/Courbes/Matrice3D/${FICHIER:0}.pdf
end=`date +%s`
runtime=$((end-start))
echo "Fin du script, l'execution a durée" $((runtime/60)) "minutes."
