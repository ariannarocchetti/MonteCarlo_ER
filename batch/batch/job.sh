#!/bin/bash

#SBATCH --job-name=XENONnT_ER
#SBATCH --output=job-prompt.txt

#SBATCH --ntasks=1
#SBATCH --array=1-10

#source ~/.bashrc
cd ..
#loadgeant4
user=$(whoami)
mkdir /scratch/$user/$5/ -p
tmp=/scratch/$user/$5/"output_$2_$3_${SLURM_ARRAY_TASK_ID}.root"
dst=$1/"output_$2_$3_${SLURM_ARRAY_TASK_ID}.root"
source /opt/geant/v10.3.3/bin/geant4.sh && source /opt/geant/v10.3.3/share/Geant4-10.3.3/geant4make/geant4make.sh && export G4WORKDIR=.
./bin/Linux-g++/xenon1t_G4p10 -p /users/arocchetti/mc/macros/XENONnT/preinit_TPC.mac -f $1/"run_ER_$2_$3.mac" -n $4 -o $tmp -d XENONnT
mv $tmp $dst
