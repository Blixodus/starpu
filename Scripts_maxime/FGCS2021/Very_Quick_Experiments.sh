# A lancer dans le dossier starpu/ avec bash Scripts_maxime/FGCS2021/Very_Quick_Experiments.sh name_grid5k
NAME=$1
./configure --prefix=/home/${NAME}/starpu
make -j 100
make install

# Dans l'article
#~ bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 2 Matrice_ligne HFP 1
#~ mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_M2D.txt
#~ mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_M2D.txt
#~ bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 2 Matrice_ligne HFP_memory 1
#~ mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_MEMORY_M2D.txt
#~ bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 2 Matrice3DZN HFP 1
#~ mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_M3DZN.txt
#~ mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_M3DZN.txt
#~ bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 2 Cholesky HFP 1
#~ mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_CHO.txt
#~ mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_CHO.txt
#~ bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 2 Random_task_order HFP 1
#~ mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_M2D_RANDOM_ORDER.txt
#~ mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_M2D_RANDOM_ORDER.txt
#~ bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 2 Random_tasks HFP 1
#~ mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_M2D_RANDOM_TASKS.txt
#~ mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_M2D_RANDOM_TASKS.txt


# Pour la révision
bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 2 Sparse HFP 1
mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_SPARSE.txt
mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_SPARSE.txt

bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 2 Sparse HFP_N15 1
mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_N15_SPARSE.txt
mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_N15_SPARSE.txt

bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 2 Sparse HFP_N40 1
mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_N40_SPARSE.txt
mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_N40_SPARSE.txt

bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 2 Sparse3DZN HFP 1
mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_SPARSE_3DZN.txt
mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_SPARSE_3DZN.txt

bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 2 Sparse3DZN HFP_N6 1
mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_N6_SPARSE_3DZN.txt
mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_N6_SPARSE_3DZN.txt

bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 2 Sparse3DZN HFP_N14 1
mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_N14_SPARSE_3DZN.txt
mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_N14_SPARSE_3DZN.txt


# Pas dans l'article
#~ bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 6 Matrice3D HFP 1
#~ mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_M3D.txt
#~ mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_M3D.txt
#~ bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 4 Sparse HFP 1
#~ mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_SPARSE.txt
#~ mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_SPARSE.txt
#~ bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 4 Sparse HFP_mem_infinie 1
#~ mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_SPARSE_INFINIE.txt
#~ mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_SPARSE_INFINIE.txt
