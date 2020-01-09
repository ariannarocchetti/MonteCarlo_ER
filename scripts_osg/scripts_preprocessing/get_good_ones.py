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

isotope_array = [#"U238",
                "Co60",
                "K40",
                "Cs137",
                "Th228",
                "U235",
                "Th232",
                "Ra226",
                #"geantinos"
                ]
material_array = ["Copper_FieldGuard_"]
os.system("source activate pax_head")
storage_path = "/dali/lgrandi/xenonnt/simulations/er_simulations"
destination = storage_path + "/good_files_to_copy"
os.makedirs(storage_path, exist_ok=True)

for material in material_array:
    for isotope in isotope_array:
        dir_name_for_storage = storage_path + "/final_files/"+material+"/"+isotope
        name_final_file = dir_name_for_storage +"/output_"+ material + '_' + isotope +  '_FINAL'+".root"
        os.system("cp %s %s" %(name_final_file, destination))

