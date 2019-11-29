#!/usr/bin/python

import sys
import os
from datetime import date

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

#isotope_array = ["geantinos"]
EVENT_COUNT = 10000000

#DATE_STRING = str(date.today())
DATE_STRING = "2019-11-25"
counter = 0
file_corrupted = 0
file_ok = 0
for MATERIAL_STRING in material_array:
    
    for ISOTOPE_STRING in isotope_array:
        
        PATH ="/sc/userdata/arocchetti/XENONnT_"+ DATE_STRING + "/" + MATERIAL_STRING + "/" + ISOTOPE_STRING
        os.makedirs(PATH, exist_ok=True)
        
        for file in os.listdir(PATH):
            
            if file.endswith(".out"):
                if file.endswith("Sort.out"):
                    break
                
                counter = counter +1
                path_file = os.path.join(PATH, file)
                fp = open(path_file, 'r')
            
                #if "Total number of events requested: 100000" in fp.read():
                if "fileMerger::CleanUp() Done" in fp.read():
                    file_ok = file_ok +1
                #print(" ---> ", path_file, "----> OK!!")
                    fp.close()
                else:
                    fp = open(path_file, 'r')
                    lineList = fp.readlines()
                    print(lineList[-1])
                    print("check ---> ", path_file)
                    file_corrupted = file_corrupted +1
                    #os.remove(path_file) 
                    fp.close()
              
print ("generation of :", EVENT_COUNT, "for ", len(material_array), "materials and ", len(isotope_array), "isotopes")
print("file success:", file_ok, "\ ", counter)
print("file bad :", file_corrupted)
#print("material list:", material_array)
#print("isotope array", isotope_array)
