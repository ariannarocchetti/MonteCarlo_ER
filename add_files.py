#!/usr/bin/python

import sys
import os
from datetime import date

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
material_array = ["SS_OuterCryostat"]
#DATE_STRING = str(date.today())
DATE_STRING = "2019-11-19"
##### ##### #####
N_FILES = 10
flag = 0 #0 doesn't move the files, 1 does . 


localfile = "20191125T042742-0600"
mc_dir =  "/scratch/arianna/er/processing/montecarlo/output/ariannarocchetti/pegasus/montecarlo/"
scp_path = "ariannarocchetti@login.xenon.ci-connect.net"
storage_dir = "/sc/CASTOR2/user/a/arocchetti/storage_osg"


os.system("ls /sc/CASTOR2")
os.system("scp -r %s:%s/%s" % (scp_path, mc_dir) )
os.system("for f in *; do tar xf $f; done")


for material in material_array:
    for isotope in isotope_array:

        PATH ="/sc/userdata/arocchetti/XENONnT_"+ DATE_STRING + "/" + material + "/" + isotope
        #os.makedirs(PATH, exist_ok=True)
        name = PATH+"/output_"+ material + '_' + isotope +  '_FINAL'+"_Sort.root"
        files_to_add = PATH+"/output_"+ material + '_' + isotope +  '_*_Sort' + '.root'
        dest="/sc/CASTOR2/user/a/arocchetti/storage/"+DATE_STRING+"/" + material + "/" + isotope 
        os.makedirs(dest, exist_ok=True)
        

        os.system("hadd %s %s"%(name, files_to_add))
        
        if flag==1:     
            for i in range(0, N_FILES):
                file_to_move=PATH+"/output_"+ material + '_' + isotope +  '_' + str(i) + '.root'
                print(file_to_move)
                os.system("mv %s %s"%(file_to_move, dest))

                

