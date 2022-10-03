NAME=$1
./configure --prefix=/home/${NAME}/starpu
make -j 100
make install

bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 18 Matrice_ligne HFP 1
mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_M2D.txt
mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_M2D.txt
bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 10 Matrice_ligne HFP_memory 1
mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_MEMORY_M2D.txt
bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 10 Matrice3DZN HFP 1
mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_M3DZN.txt
mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_M3DZN.txt
bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 10 Cholesky HFP 1
mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_CHO.txt
mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_CHO.txt
bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 16 Random_task_order HFP 1
mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_M2D_RANDOM_ORDER.txt
mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_M2D_RANDOM_ORDER.txt
bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 16 Random_tasks HFP 1			
mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_M2D_RANDOM_TASKS.txt
mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_M2D_RANDOM_TASKS.txt
bash Scripts_maxime/PlaFRIM-Grid5k/FGCS.sh 18 Sparse HFP 1 # Sparse 10%
mv Output_maxime/GFlops_raw_out_1.txt Output_maxime/Data/GF_HFP_SPARSE.txt
mv Output_maxime/GFlops_raw_out_3.txt Output_maxime/Data/DT_HFP_SPARSE.txt
