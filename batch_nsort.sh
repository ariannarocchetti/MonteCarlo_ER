#!/bin/bash
#SBATCH --job-name=nSort
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --mem-per-cpu=4480
#SBACTH --output=log.out
#SBATCH --time=8:00:00
#SBATCH --error=err.out
#SBATCH --account=pi-lgrandi
#SBATCH --partition=dali
#SBATCH --qos=dali

export PATH="/cvmfs/xenon.opensciencegrid.org/releases/anaconda/2.4/bin:$PATH"
source activate pax_head

python /home/rocchetti/mc/nSort/nSort_stage.py  /dali/lgrandi/xenonnt/simulations/er_simulations/raw_files/20191202T162318-0600
ECHO "DONE!"
