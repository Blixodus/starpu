#!/usr/bin/bash
start=`date +%s`
ulimit -S -s 5000000
make -C src/ -j 100

N=10
NGPU=3
#~ ORDO="dynamic-data-aware"
#~ ORDO="dmdar"
#~ ORDO="eager"
#~ BW=10726
CM=500
#~ EVICTION=0
EVICTION=1
READY=0
#~ READY=1
TH=10
CP=5
HOST="gemini-1-ipdps"
#~ HOST="gemini-2-ipdps"
#~ HOST="attila"
export STARPU_PERF_MODEL_DIR=tools/perfmodels/sampling
NB_TAILLE_TESTE=2
FICHIER_BUS=Output_maxime/GFlops_raw_out_2.txt
FICHIER_RAW_DT=Output_maxime/GFlops_raw_out_3.txt
truncate -s 0 ${FICHIER_BUS}
truncate -s 0 ${FICHIER_RAW_DT}
NCOMBINAISONS=$((NGPU*2+(NGPU-1)*NGPU+3))


echo "############## DD ##############"
for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do 
	N=$((5*i))
	STARPU_SCHED=dmdar STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_LIMIT_CUDA_MEM=$((CM)) STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 STARPU_HOSTNAME=${HOST} STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS:0}" ./examples/mult/sgemm -xy $((960*N)) -nblocks $((N)) -iter 1
	sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS:0} >> ${FICHIER_RAW_DT:0}
done
echo "############## Modular eager prefetching ##############"
for ((i=1 ; i<=(($NB_TAILLE_TESTE)); i++))
	do 
	N=$((5*i))
	STARPU_SCHED=modular-eager-prefetching STARPU_NTASKS_THRESHOLD=$((TH)) STARPU_CUDA_PIPELINE=$((CP)) STARPU_LIMIT_CUDA_MEM=$((CM)) STARPU_MINIMUM_CLEAN_BUFFERS=0 STARPU_TARGET_CLEAN_BUFFERS=0 STARPU_NCPU=0 STARPU_NCUDA=$((NGPU)) STARPU_NOPENCL=0 STARPU_HOSTNAME=${HOST} STARPU_BUS_STATS=1 STARPU_BUS_STATS_FILE="${FICHIER_BUS:0}" ./examples/mult/sgemm -xy $((960*N)) -nblocks $((N)) -iter 1
	sed -n '4,'$((NCOMBINAISONS))'p' ${FICHIER_BUS:0} >> ${FICHIER_RAW_DT:0}
done

gcc -o cut_datatransfers_raw_out cut_datatransfers_raw_out.c
./cut_datatransfers_raw_out $NB_TAILLE_TESTE 2 5 0 $NGPU ${FICHIER_RAW_DT:0} Output_maxime/test.txt

end=`date +%s`
runtime=$((end-start))
echo "Fin du script, l'execution a durée" $((runtime/60))" min "$((runtime%60))" sec."
