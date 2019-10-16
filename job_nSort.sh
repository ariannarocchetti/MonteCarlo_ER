#/bin/bash

#SBATCH --job-name=Xe-nSort
#SBATCH --output=job_nSort-prompt.txt

#SBATCH --ntasks=1
#SBATCH --array=1-10

source ~/.bashrc
cd ..
user=$(whoami)
mkdir /scratch/$user -p
tmp=/scratch/$user/"output_$2_$3_${SLURM_ARRAY_TASK_ID}"
dst=$1/"output_$2_$3_${SLURM_ARRAY_TASK_ID}"
#./users/arocchetti/mc/nSort/nSort - -i ../events 2 1 0 0 0 -d XENONnT 2 1 0 0 0 
./users/arocchetti/mc/nSort/nSort - $1/"output_$2_$3_${SLURM_ARRAY_TASK_ID}" 2 1 0 0 0 -d XENONnT 
mv $tmp $dst
