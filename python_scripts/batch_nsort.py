x = """
#!/bin/bash
#SBATCH --job-name=nSort
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=8
#SBATCH --mem-per-cpu=4480
#SBACTH --output=log.out
#SBATCH --time=8:00:00

#SBATCH --account=pi-lgrandi
#SBATCH --partition=dali
#SBATCH --qos=dali

export PATH="/project2/lgrandi/anaconda3/bin:$PATH"
source activate pax_head

python /home/rocchetti/mc/nSort/nSort_stage.py ${STORAGE_PATH}/${DIR}
"""
import sys
import os
from datetime import date
#import tqdm
import glob

import subprocess
from pexpect import pxssh
import sys
from cax.qsub import submit_job

storage_path = "/dali/lgrandi/xenonnt/simulations/er_simulations"
command = ("ls -d %s/raw_files/*"%storage_path)
list_=subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
(out, err)=list_.communicate()
out = out.decode("utf-8")
out = out.split("\n")

for directory_name in out[3:4]:
    y = x.format(DIR=directory_name,STORAGE_PATH=storage_path )
    print(directory_name)
    submit_job(y)       
