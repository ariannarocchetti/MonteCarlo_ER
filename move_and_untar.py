#!/usr/bin/python

import sys
import os
from datetime import date
#import tqdm

import subprocess
from pexpect import pxssh
##### INPUT PARAMETER #####

material_array = ["SS_OuterCryostat",
                "SS_InnerCryostat",
                "OuterCryostatReflector",
                "SS_BellPlate",
                "SS_BellSideWall",
                "PmtTpc",
                "Copper_TopRing",
                "Copper_LowerRing",
                "Teflon_Pillar_",
                "Copper_FieldGuard_",
                "Copper_FieldShaperRing_",
                "SS_AnodeRing",
                "Teflon_BottomTPC",
                "Teflon_TPC",
                "Teflon_TopElectrodesFrame",
                "Copper_BottomPmtPlate",
                "Copper_TopPmtPlate",
                ]

isotope_array = ["U238",
                "Co60",
                "K40",
                "Cs137",
                "Th228",
                "U235",
                "Th232",
                "Ra226",
                #"geantinos"
                ]

EVENT_COUNT = 100000
#material_array = ["SS_InnerCryostat"]
#isotope_array = ["Th232"]
#DATE_STRING = str(date.today())
DATE_STRING = "20191204"
##### ##### #####

localfile = DATE_STRING+"T"
mc_dir =  "/scratch/arianna/er/processing/montecarlo/output/ariannarocchetti/pegasus/montecarlo/"
scp_path = "ariannarocchetti@login.xenon.ci-connect.net"
scp_domain="login.xenon.ci-connect.net"

os.system("source activate pax_head")
#get list of directories to copy

command = ("ssh ariannarocchetti@login.xenon.ci-connect.net ls  /scratch/arianna/er/processing/montecarlo/output/ariannarocchetti/pegasus/montecarlo/")
#command = ("ls -d raw_files/*")
list_=subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
(out, err)=list_.communicate()
out = out.decode("utf-8")
out = out.split("\n")
storage_path = "/dali/lgrandi/xenonnt/simulations/er_simulations" 
os.makedirs(storage_path, exist_ok=True)

for i in range(0, len(out)-2):
     os.system("scp -r %s:%s/%s %s" % (scp_path, mc_dir,out[i],storage_path) )
     os.system("cp untar.sh %s" %out[i])
     os.system("chmod u+x %s/untar.sh"%out[i])
     os.chdir("%s/%s" %(storage_path, out[i]))
     print("----------------untar----------------------")
     os.system("pwd")
     os.system("./untar.sh")
     print("---------------------done!-----------------")
     os.chdir("%s" %storage_path)
     os.system("cp %s/*_Sort.root %s" %(out[i], storage_path ))   

for material in material_array:
    for isotope in isotope_array:
        print("working on:", material, isotope)
        osg_file_name = "Xenon1T_ER_" + material+ "_" + isotope + "_*" + ".root"
##########make directory for a nice storage tree
        dir_name_for_storage = storage_path + "/final_files/"+material+"/"+isotope
        os.makedirs(dir_name_for_storage, exist_ok = True)

##########move files in the right directory
        os.system("mv %s/%s %s" %(storage_path,osg_file_name, dir_name_for_storage))
##########hadd nSorted files 

        name_final_file = dir_name_for_storage +"/output_"+ material + '_' + isotope +  '_FINAL'+"_Sort.root"
        files_name = "Xenon1T_ER_" + material+ "_" + isotope + "*" + "_Sort.root"
        os.system("hadd -f %s %s"%(name_final_file, name_final_file))

