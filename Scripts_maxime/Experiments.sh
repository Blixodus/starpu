#!/usr/bin/bash
start=`date +%s`
#~ ./configure --enable-simgrid --disable-mpi --with-simgrid-dir=/home/gonthier/simgrid
sudo make install
sudo make -j4
bash Scripts_maxime/Matrice_ligne/GF_M_MC_NT=225_LRU_BW350.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 2
bash Scripts_maxime/Matrice_ligne/GF_NT_MC_LRU_BW350_CM500.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 2
echo "Scripts de matrice 2D fini"
bash Scripts_maxime/Matrice3D/GF_M_M3D_N=15_BW350.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 2
bash Scripts_maxime/Matrice3D/GF_NT_M3D_BW350_CM500.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 2
echo "Scripts de matrice 3D fini"
bash Scripts_maxime/Random_tasks/GF_M_MC_LRU_N=15_BW350_CM500_RANDOMTASKS.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 2
bash Scripts_maxime/Random_tasks/GF_NT_MC_LRU_BW350_CM500_RANDOMTASKS.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 2
echo "Scripts de random matrice 2D fini"
bash Scripts_maxime/Cholesky/GF_M_CHO_N=20_BW350.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 2
bash Scripts_maxime/Cholesky/GF_NT_CHO_BW350_CM500.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 2
echo "Scripts de cholesky fini"
end=`date +%s`
runtime=$((end-start))
echo "Fin du script, l'execution a durée" $((runtime/60))" min "$((runtime%60))" sec."


#!/usr/bin/bash
#~ start=`date +%s`
#~ ./configure --enable-simgrid --disable-mpi --with-simgrid-dir=/home/gonthier/simgrid
#~ sudo make install
#~ sudo make -j4
#~ bash Scripts_maxime/Matrice_ligne/GF_M_MC_NT=225_LRU_BW350.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 10
#~ bash Scripts_maxime/Matrice_ligne/GF_NT_MC_LRU_BW350_CM500.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 10
#~ echo "Scripts de matrice 2D fini"
#~ bash Scripts_maxime/Matrice3D/GF_M_M3D_N=15_BW350.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 10
#~ bash Scripts_maxime/Matrice3D/GF_NT_M3D_BW350_CM500.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 8
#~ echo "Scripts de matrice 3D fini"
#~ bash Scripts_maxime/Random_tasks/GF_M_MC_LRU_N=15_BW350_CM500_RANDOMTASKS.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 10
#~ bash Scripts_maxime/Random_tasks/GF_NT_MC_LRU_BW350_CM500_RANDOMTASKS.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 10
#~ echo "Scripts de random matrice 2D fini"
#~ bash Scripts_maxime/Cholesky/GF_M_CHO_N=20_BW350.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 9
#~ bash Scripts_maxime/Cholesky/GF_NT_CHO_BW350_CM500.sh /home/gonthier/ /home/gonthier/these_gonthier_maxime/Starpu/ 7
#~ echo "Scripts de cholesky fini"
#~ end=`date +%s`
#~ runtime=$((end-start))
#~ echo "Fin du script, l'execution a durée" $((runtime/60))" min "$((runtime%60))" sec."
