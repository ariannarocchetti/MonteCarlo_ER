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
                "SS_GateRing",
                "SS_AnodeRing",
                "SS_TopMeshRing",
                "SS_CathodeRing",
                "SS_BottomMeshRing",
                "Teflon_BottomTPC",
                "Teflon_TPC",
                "GXeTeflon_TopElectrodesFrame",
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
#material_array = ["SS_OuterCryostat"]
#isotope_array = ["Th232"]
#DATE_STRING = str(date.today())
DATE_STRING = "20191127"
##### ##### #####

flag = 0 #0 doesn't move the files, 1 does . 


localfile = DATE_STRING+"T"
mc_dir =  "/scratch/arianna/er/processing/montecarlo/output/ariannarocchetti/pegasus/montecarlo/"
scp_path = "ariannarocchetti@login.xenon.ci-connect.net"
storage_dir = "/sc/CASTOR2/user/a/arocchetti/storage_osg"
scp_domain="login.xenon.ci-connect.net"


#get list of directories to copy

command = ("ssh ariannarocchetti@login.xenon.ci-connect.net ls  /scratch/arianna/er/processing/montecarlo/output/ariannarocchetti/pegasus/montecarlo/")
list_=subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
(out, err)=list_.communicate()
out = out.decode("utf-8")
out = out.split("\n")

for i in range(0, len(out)-1):
    string_dir_to_get = scp_path+mc_dir+out[i]

    storage_path = "/sc/CASTOR2/user/a/arocchetti/storage_osg/"+DATE_STRING
    os.makedirs(storage_path, exist_ok=True)
    os.system("ls /sc/CASTOR2")
    if flag == 1:
        os.system("scp -r %s:%s/%s %s" % (scp_path, mc_dir,out[i],storage_dir) )
        print(".......extracting.....")
        os.system("for f in %s/%s/*; do tar xf $f -C %s; done" %(storage_dir, out[i], storage_path))
    else:
        print("file already extracted")
for material in material_array:
    for isotope in isotope_array:
        print("working on:", material, isotope)
        osg_file_name = "Xenon1T_ER_" + material+ "_" + isotope + "_*" + ".root"
##########make directory for a nice storage tree
        dir_name_for_storage = storage_path + "/"+material+"/"+isotope
        os.makedirs(dir_name_for_storage, exist_ok = True)
        
##########move files in the right directory
        if flag == 1:
            os.system("mv %s/%s %s" %(storage_path,osg_file_name, dir_name_for_storage))
        else:
            print("files already in dir")
##########hadd nSorted files 

        PATH ="/sc/userdata/arocchetti/XENONnT_"+ DATE_STRING + "/" + material + "/" + isotope  #for analysis
        os.makedirs(PATH, exist_ok=True) #for analysis
        print("made dir:", PATH)
        name_final_file = PATH+"/output_"+ material + '_' + isotope +  '_FINAL'+"_Sort.root"
        files_name =  "Xenon1T_ER_" + material+ "_" + isotope + "*" + "_Sort.root"
        files_to_add = dir_name_for_storage+"/"+files_name
        os.system("hadd -f %s %s"%(name_final_file, files_to_add))
print("DONE!")




"""
s = pxssh.pxssh(encoding='utf-8')
if not s.login(scp_domain, 'ariannarocchetti', 'Akyraz0eBella'):

    print ("SSH session failed on login.")
    print (str(s))

else:
    print ("SSH session login successful  ariannarocchetti@%s"%(scp_domain) )
    command=("list_of_dir = glob.glob(string_dir_to_get)")
    s.sendline(command)
    s.prompt()
    s.logout()
    #print("list_of_dirs :", list_of_dir)
"""
