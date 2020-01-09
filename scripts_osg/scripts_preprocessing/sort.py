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

material_array = ["SS_BellPlate" ]
#isotope_array = ["Th232"]
#DATE_STRING = str(date.today())
##### ##### #####

os.system("source activate pax_head")

command = ("ls -d raw_files/20191204*")
list_=subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
(out, err)=list_.communicate()
out = out.decode("utf-8")
out = out.split("\n")
storage_path = "/dali/lgrandi/xenonnt/simulations/er_simulations" 
os.makedirs(storage_path, exist_ok=True)
untar =0


for i in range(0,len(out)-1):
     
     os.system("cp untar.sh %s" %out[i])
     os.system("chmod u+x %s/untar.sh"%out[i])
     os.system("pwd")
     if untar==0:
         print("files already extracted")
     if untar==1:
         os.chdir("%s/%s" %(storage_path, out[i]))
         os.system("./untar.sh")
         print("---------------------done!-----------------")
         os.chdir("%s" %storage_path)
     
     print("Copying files:%s" %out[i])
     print("cp %s/*Sort.root %s" %(storage_path+"/"+out[i], storage_path ))
     os.system("cp %s/*Sort.root %s" %(storage_path+"/"+out[i], storage_path ))   


for material in material_array:
    for isotope in isotope_array:
        print("working on:", material, isotope)
        osg_file_name = "Xenon1T_ER_" + material+ "_" + isotope + "_Sort" + ".root"
##########make directory for a nice storage tree
        dir_name_for_storage = storage_path + "/final_files/"+material+"/"+isotope
        os.makedirs(dir_name_for_storage, exist_ok = True)

##########move files in the right directory
        #os.system("mv %s/%s %s" %(storage_path,osg_file_name, dir_name_for_storage))
##########hadd nSorted files 

        name_final_file = dir_name_for_storage +"/output_"+ material + '_' + isotope +  '_FINAL'+".root"
        files_name =  "Xenon1T_ER_" + material+ "_" + isotope + "*" + "Sort.root"
        os.system("hadd -f -k %s %s"%(name_final_file, files_name))

os.system("rm %s/Xenon*" %storage_path)

