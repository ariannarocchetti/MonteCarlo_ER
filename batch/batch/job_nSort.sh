#!/bin/bash
#SBATCH --job-name=Xe-nSort
#SBATCH --output=job_nSort-prompt.txt
#SBATCH --ntasks=1
#SBATCH --array=1-10
#source ~/.bashrc

user=$(whoami)


#mkdir /scratch/$user/ -p
tmp=$1/"output_$2_$3_${SLURM_ARRAY_TASK_ID}_Sort.root"
mkdir /sc/CASTOR2/user/a/arocchetti/$2/$3/ -p
dst=/sc/CASTOR2/user/a/arocchetti/$2/$3/"output_$2_$3_${SLURM_ARRAY_TASK_ID}_Sort.root"
./../nSort/nSort -i $1/"output_$2_$3_${SLURM_ARRAY_TASK_ID}" -d XENONnT 2 1 0 0 0  
mv $tmp $dst
