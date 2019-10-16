#!/usr/bin/python

import sys
import os
from datetime import date

##### INPUT PARAMETER #####
#material_array = ["SS_OuterCryostat"]
material_array = ["SS_OuterCryostat","SS_InnerCryostat", "OuterCryostatReflector", "SS_BellPlate", "SS_BellSideWall", "PmtTpc*", "Copper_TopRing","Copper_LowerRing", "Teflon_Pillar_*", "Copper_FieldGuard_*", "Copper_FieldShaperRing_*"]

# Full list of materials: ["PTFE", "Cu", "Cirlex", "Ti", "CryoMat", "PMT"]
isotope_array = ["U238Pb206", "Co60", "K40", "Cs137"]
# Full list of Isotopes:  ["U238Pb206", "Co60", "K40", "Cs137", "Ag110m", "Th232Pb208", "U235Pb207", "Ti44Sc44Ca44"]
EVENT_COUNT = 10000
#POSTPONE_DECAY = ["true"]
DATE_STRING = str(date.today())
#print(DATE_STRING)
##### ##### #####
#DATE_STRING = "2019-10-14"
counter = 0
file_ok = 0
for MATERIAL_STRING in material_array:
    
    for ISOTOPE_STRING in isotope_array:
        
        PATH ="/sc/userdata/arocchetti/XENONnT_"+ DATE_STRING + "/" + MATERIAL_STRING + "/" + ISOTOPE_STRING
        os.makedirs(PATH, exist_ok=True)
        
        for file in os.listdir(PATH):
            
            if file.endswith(".out"):
                counter = counter +1
                #print(os.path.join(PATH, file))
                path_file = os.path.join(PATH, file)
                fp = open(path_file, 'r')
                if "fileMerger::CleanUp() Done" in fp.read():
                    # lineList = fp.readlines()
                #if (lineList[-1]== "fileMerger::CleanUp() Done"):
                    print( "-----", path_file, "-----OK!")
                    file_ok = file_ok +1
               # else:
                    #lineList = fp.readlines()
                    #print( "Running at ---> ", file # lineList[-1], "\n")
                fp.close()
              
print ("generation of :", EVENT_COUNT, "for ", len(material_array), "materials and ", len(isotope_array), "isotopes")
print("file success:", file_ok, "\ ", counter)
#print("material list:", material_array)
#print("isotope array", isotope_array)
