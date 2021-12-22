# bash Scripts_maxime/PlaFRIM-Grid5k/Recup_data_Rebuttal.sh 10 Matrice3D dynamic_data_aware_no_hfp 1
# bash Scripts_maxime/PlaFRIM-Grid5k/Recup_data_Rebuttal.sh 8 Matrice3D dynamic_data_aware_no_hfp 2
# bash Scripts_maxime/PlaFRIM-Grid5k/Recup_data_Rebuttal.sh 10 Cholesky dynamic_data_aware_no_hfp 1
# bash Scripts_maxime/PlaFRIM-Grid5k/Recup_data_Rebuttal.sh 7 Cholesky dynamic_data_aware_no_hfp 2
# bash Scripts_maxime/PlaFRIM-Grid5k/Recup_data_Rebuttal.sh 15 Matrice_ligne dynamic_data_aware_no_hfp_sparse_matrix 1
# bash Scripts_maxime/PlaFRIM-Grid5k/Recup_data_Rebuttal.sh 5 Matrice_ligne dynamic_data_aware_no_hfp_sparse_matrix 2

# Vraiment pour le rebuttal
# bash Scripts_maxime/PlaFRIM-Grid5k/Recup_data_Rebuttal.sh 14 Cholesky dynamic_data_aware_no_hfp 4
# bash Scripts_maxime/PlaFRIM-Grid5k/Recup_data_Rebuttal.sh 15 Sparse dynamic_data_aware_no_hfp 4
# bash Scripts_maxime/PlaFRIM-Grid5k/Recup_data_Rebuttal.sh 15 Sparse_mem_infinite dynamic_data_aware_no_hfp 4

NB_TAILLE_TESTE=$1
DOSSIER=$2
MODEL=$3
NGPU=$4
START_X=0
GPU=gemini-1-fgcs
PATH_R=/home/gonthier/these_gonthier_maxime/Starpu
PATH_STARPU=/home/gonthier
ECHELLE_X=$((5))
NITER=11

# NB_ALGO_TESTE et fichiers à récupérer
if [ $DOSSIER == "Matrice_ligne" ]
then
	FICHIER_GF=GF_HFP_M2D_${NGPU}GPU.txt
	FICHIER_DT=DT_HFP_M2D_${NGPU}GPU.txt
	NB_ALGO_TESTE=8
fi
if [ $DOSSIER == "Matrice3D" ]
then
	FICHIER_GF=GF_HFP_M3D_${NGPU}GPU.txt
	FICHIER_DT=DT_HFP_M3D_${NGPU}GPU.txt
	NB_ALGO_TESTE=11
fi
if [ $DOSSIER == "Cholesky" ]
then
	FICHIER_GF=GF_HFP_CHO_${NGPU}GPU.txt
	FICHIER_DT=DT_HFP_CHO_${NGPU}GPU.txt
	NB_ALGO_TESTE=5
fi
if [ $DOSSIER == "Sparse" ]
then
	ECHELLE_X=$((50))
	FICHIER_GF=GF_HFP_SPARSE_${NGPU}GPU.txt
	FICHIER_DT=DT_HFP_SPARSE_${NGPU}GPU.txt
	NB_ALGO_TESTE=5
fi
if [ $DOSSIER == "Sparse_mem_infinite" ]
then
	ECHELLE_X=$((50))
	FICHIER_GF=GF_HFP_SPARSE_INFINIE_${NGPU}GPU.txt
	FICHIER_DT=DT_HFP_SPARSE_INFINIE_${NGPU}GPU.txt
	NB_ALGO_TESTE=5
fi

# + 1 ALGO TESTE SI MULTI GPU
if [ $NGPU != 1 ]
then
	NB_ALGO_TESTE=$((NB_ALGO_TESTE+1))
fi

#~ scp mgonthier@access.grid5000.fr:/home/mgonthier/lyon/starpu/Output_maxime/${FICHIER_GF} /home/gonthier/starpu/Output_maxime/Data/${DOSSIER}/${FICHIER_GF}
#~ scp mgonthier@access.grid5000.fr:/home/mgonthier/lyon/starpu/Output_maxime/${FICHIER_DT} /home/gonthier/starpu/Output_maxime/Data/${DOSSIER}/${FICHIER_DT}

# Tracage des GFlops
#~ gcc -o cut_gflops_raw_out cut_gflops_raw_out.c
#~ ./cut_gflops_raw_out $NB_TAILLE_TESTE $NB_ALGO_TESTE $ECHELLE_X $START_X /home/gonthier/starpu/Output_maxime/Data/${DOSSIER}/${FICHIER_GF} ${PATH_R}/R/Data/PlaFRIM-Grid5k/${DOSSIER}/GF_${MODEL}_${GPU}_${NGPU}GPU.txt
Rscript ${PATH_R}/R/ScriptR/GF_X.R ${PATH_R}/R/Data/PlaFRIM-Grid5k/${DOSSIER}/GF_${MODEL}_${GPU}_${NGPU}GPU.txt ${MODEL} ${DOSSIER} ${GPU} ${NGPU} ${NITER}
# Rscript ${PATH_R}/R/ScriptR/test.R ${PATH_R}/R/Data/PlaFRIM-Grid5k/${DOSSIER}/GF_${MODEL}_${GPU}_${NGPU}GPU.txt ${MODEL} ${DOSSIER} ${GPU} ${NGPU} ${NITER}
mv ${PATH_STARPU}/starpu/Rplots.pdf ${PATH_R}/R/Courbes/PlaFRIM-Grid5k/${DOSSIER}/GF_${MODEL}_${GPU}_${NGPU}GPU.pdf

# Tracage data transfers
#~ gcc -o cut_datatransfers_raw_out cut_datatransfers_raw_out.c
#~ ./cut_datatransfers_raw_out $NB_TAILLE_TESTE $NB_ALGO_TESTE $ECHELLE_X $START_X $NGPU /home/gonthier/starpu/Output_maxime/Data/${DOSSIER}/${FICHIER_DT} ${PATH_R}/R/Data/PlaFRIM-Grid5k/${DOSSIER}/DT_${MODEL}_${GPU}_${NGPU}GPU.txt
#~ Rscript ${PATH_R}/R/ScriptR/GF_X.R ${PATH_R}/R/Data/PlaFRIM-Grid5k/${DOSSIER}/DT_${MODEL}_${GPU}_${NGPU}GPU.txt DT_${MODEL} ${DOSSIER} ${GPU} ${NGPU} ${NITER}
#~ mv ${PATH_STARPU}/starpu/Rplots.pdf ${PATH_R}/R/Courbes/PlaFRIM-Grid5k/${DOSSIER}/DT_${MODEL}_${GPU}_${NGPU}GPU.pdf

