#!/bin/bash

#SBATCH --job-name=Xe-nSort
#SBATCH --output=job_nSort-prompt.txt

#SBATCH --ntasks=1
#SBATCH --array=1

source ~/.bashrc
cd ..

user=$(whoami)
mkdir /scratch/$user -p
tmp=/scratch/$user/"output_$2_$3_${SLURM_ARRAY_TASK_ID}"
dst=/sc/CASTOR2/user/a/arocchetti/sorted_files/"output_$2_$3_${SLURM_ARRAY_TASK_ID}"
./../nSort/nSort -i /scratch/$user/$5/"output_$2_$3_${SLURM_ARRAY_TASK_ID}" -o $tmp -d XENONnT 2 1 0 0 0  
mv $tmp $dst
