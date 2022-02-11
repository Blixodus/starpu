# 	bash Scripts_maxime/PlaFRIM-Grid5k/Recup_data_Mutex_DARTS.sh 12 Matrice_ligne mutex_darts 4
#	bash Scripts_maxime/PlaFRIM-Grid5k/Recup_data_Mutex_DARTS.sh 3 Matrice3D mutex_darts 4
#	bash Scripts_maxime/PlaFRIM-Grid5k/Recup_data_Mutex_DARTS.sh 3 Cholesky mutex_darts 4

NB_TAILLE_TESTE=$1
DOSSIER=$2
MODEL=$3
NGPU=$4
START_X=0
GPU=gemini-1-fgcs
PATH_R=/home/gonthier/these_gonthier_maxime/Starpu
PATH_STARPU=/home/gonthier
ECHELLE_X=$((5*NGPU))
NITER=11

if [ $DOSSIER == "Matrice_ligne" ]
then
	NB_ALGO_TESTE=2
	
	FICHIER_GF=GF_mutex_M2D_${NGPU}GPU.txt

	FICHIER_GF_REFINED=GF_refined_mutex_M2D_${NGPU}GPU.txt
	FICHIER_CONFLITS_REFINED=Nb_conflit_donnee_refined_mutex_M2D_${NGPU}GPU.txt
	FICHIER_CONFLITS_CRITIQUE_REFINED=Nb_conflit_donnee_critique_refined_mutex_M2D_${NGPU}GPU.txt
	
	FICHIER_GF_LINEAR=GF_linear_mutex_M2D_${NGPU}GPU.txt
fi
if [ $DOSSIER == "Matrice3D" ]
then
	NB_ALGO_TESTE=4
	
	FICHIER_GF=GF_mutex_M3D_${NGPU}GPU.txt

	FICHIER_GF_REFINED=GF_refined_mutex_M3D_${NGPU}GPU.txt
	FICHIER_CONFLITS_REFINED=Nb_conflit_donnee_refined_mutex_M3D_${NGPU}GPU.txt
	FICHIER_CONFLITS_CRITIQUE_REFINED=Nb_conflit_donnee_critique_refined_mutex_M3D_${NGPU}GPU.txt
	
	FICHIER_GF_LINEAR=GF_linear_mutex_M3D_${NGPU}GPU.txt
fi
if [ $DOSSIER == "Cholesky" ]
then
	NB_ALGO_TESTE=6
	
	FICHIER_GF=GF_mutex_CHO_${NGPU}GPU.txt

	FICHIER_GF_REFINED=GF_refined_mutex_CHO_${NGPU}GPU.txt
	FICHIER_CONFLITS_REFINED=Nb_conflit_donnee_refined_mutex_CHO_${NGPU}GPU.txt
	FICHIER_CONFLITS_CRITIQUE_REFINED=Nb_conflit_donnee_critique_refined_mutex_CHO_${NGPU}GPU.txt
	
	FICHIER_GF_LINEAR=GF_linear_mutex_CHO_${NGPU}GPU.txt
fi

# Get data files from ssh
#~ scp mgonthier@access.grid5000.fr:/home/mgonthier/lyon/starpu/Output_maxime/${FICHIER_GF_REFINED} /home/gonthier/starpu/Output_maxime/Data/${DOSSIER}/${FICHIER_GF_REFINED}
#~ scp mgonthier@access.grid5000.fr:/home/mgonthier/lyon/starpu/Output_maxime/${FICHIER_CONFLITS_REFINED} /home/gonthier/starpu/Output_maxime/Data/${DOSSIER}/${FICHIER_CONFLITS_REFINED}
#~ scp mgonthier@access.grid5000.fr:/home/mgonthier/lyon/starpu/Output_maxime/${FICHIER_CONFLITS_CRITIQUE_REFINED} /home/gonthier/starpu/Output_maxime/Data/${DOSSIER}/${FICHIER_CONFLITS_CRITIQUE_REFINED}

#~ # Concaténation
#~ cat /home/gonthier/starpu/Output_maxime/Data/${DOSSIER}/${FICHIER_GF_REFINED} /home/gonthier/starpu/Output_maxime/Data/${DOSSIER}/${FICHIER_GF_LINEAR} > /home/gonthier/starpu/Output_maxime/Data/${DOSSIER}/${FICHIER_GF}

#~ # Tracage des GFlops
#~ gcc -o cut_gflops_raw_out cut_gflops_raw_out.c
#~ gcc -o cut_gflops_raw_out_csv cut_gflops_raw_out_csv.c
#~ ./cut_gflops_raw_out $NB_TAILLE_TESTE $NB_ALGO_TESTE $ECHELLE_X $START_X /home/gonthier/starpu/Output_maxime/Data/${DOSSIER}/${FICHIER_GF} ${PATH_R}/R/Data/PlaFRIM-Grid5k/${DOSSIER}/GF_${MODEL}_${GPU}_${NGPU}GPU.txt
#~ ./cut_gflops_raw_out_csv $NB_TAILLE_TESTE $NB_ALGO_TESTE $ECHELLE_X $START_X /home/gonthier/starpu/Output_maxime/Data/${DOSSIER}/${FICHIER_GF} ${PATH_R}/R/Data/PlaFRIM-Grid5k/${DOSSIER}/GF_${MODEL}_${GPU}_${NGPU}GPU.csv

# Plot FULL
python3 /home/gonthier/these_gonthier_maxime/Code/Plot.py ${PATH_R}/R/Data/PlaFRIM-Grid5k/${DOSSIER}/GF_${MODEL}_${GPU}_${NGPU}GPU.csv
mv ${PATH_STARPU}/starpu/plot.pdf ${PATH_R}/R/Courbes/PlaFRIM-Grid5k/${DOSSIER}/GF_${MODEL}_${GPU}_${NGPU}GPU.pdf

# Plot conflits de données
#~ cat /home/gonthier/starpu/Output_maxime/Data/${DOSSIER}/${FICHIER_CONFLITS_REFINED} /home/gonthier/starpu/Output_maxime/Data/${DOSSIER}/${FICHIER_CONFLITS_CRITIQUE_REFINED} > /home/gonthier/starpu/Output_maxime/Data/${DOSSIER}/Nb_conflit_merged_M2D_${NGPU}GPU.txt
#~ ./cut_gflops_raw_out_csv $NB_TAILLE_TESTE $NB_ALGO_TESTE $ECHELLE_X $START_X /home/gonthier/starpu/Output_maxime/Data/${DOSSIER}/Nb_conflit_merged_M2D_${NGPU}GPU.txt ${PATH_R}/R/Data/PlaFRIM-Grid5k/${DOSSIER}/Nb_conflit_merged_M2D_${NGPU}GPU.csv
python3 /home/gonthier/these_gonthier_maxime/Code/Plot_conflits.py ${PATH_R}/R/Data/PlaFRIM-Grid5k/${DOSSIER}/Nb_conflit_merged_M2D_${NGPU}GPU.csv
mv ${PATH_STARPU}/starpu/plot.pdf ${PATH_R}/R/Courbes/PlaFRIM-Grid5k/${DOSSIER}/GF_${MODEL}_conflits_${GPU}_${NGPU}GPU.pdf
