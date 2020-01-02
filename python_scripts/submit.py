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
#isotope_array =[ "geantinos"]

EVENT_COUNT = 10000000
#EVENT_COUNT = 10 
#POSTPONE_DECAY = ["true"]
DATE_STRING = str(date.today())
#DATE_STRING = "2019-11-07"
##### ##### #####

for MATERIAL_STRING in material_array:
    for ISOTOPE_STRING in isotope_array:

        PATH ="/sc/userdata/arocchetti/XENONnT_"+ DATE_STRING + "/" + MATERIAL_STRING + "/" + ISOTOPE_STRING
        os.makedirs(PATH, exist_ok=True)

        MACRONAME = PATH + "/" + "run_ER_" + MATERIAL_STRING + "_" + ISOTOPE_STRING + ".mac"

        f = open(MACRONAME, "w")

        f.write("#VERBOSITY" +'\n' +  "/control/verbose 0" +'\n' + "/run/verbose 0" +'\n' +"/event/verbose 0" +'\n' +"/tracking/verbose 0" +'\n' + "/xe/gun/verbose 0" +'\n' +'\n')
        #f.write('/Xe/detector/verbose 2'+ '\n')
        f.write("#SEED" +'\n' "/run/random/setRandomSeed 0" +'\n' +'\n')
        f.write("# General source settings"  +'\n' +"/xe/gun/angtype  iso" +'\n' +"/xe/gun/type  Volume" +'\n' +"/xe/gun/shape  Cylinder" +'\n' +"/xe/gun/center  0. 0. -70. cm" +'\n' + "/xe/gun/radius 100. cm" + '\n' + "/xe/gun/halfz 170. cm" + '\n' + "/xe/gun/energy 0 keV"+ '\n' + "/xe/gun/particle ion" + '\n' + '\n')
        
        #for the ones who want the i*
        if ((MATERIAL_STRING == "PmtTpc") |
           (MATERIAL_STRING == "Teflon_Pillar_") |
           (MATERIAL_STRING == "Copper_FieldGuard_")|
           (MATERIAL_STRING == "Copper_FieldShaperRing_")) :
             f.write("/xe/gun/confine " + MATERIAL_STRING + "*" + '\n')



        else:
            f.write("/xe/gun/confine " + MATERIAL_STRING + '\n')

        if ISOTOPE_STRING == "Co60": {f.write("### Co60" +'\n' +"/xe/gun/ion 27 60 0 0" +'\n'+'\n')}
        if ISOTOPE_STRING == "K40": {f.write("### K40" +'\n' +"/xe/gun/ion 19 40 0 0" +'\n'+'\n')}
        if ISOTOPE_STRING == "Cs137": {f.write("### Cs137" +'\n' +"/xe/gun/ion 55 137 0 0" +'\n'+'\n')}
	
        #splitted chain        
        if ISOTOPE_STRING == "Th228": {f.write("### Th228->Pb208 (stable)" +'\n' +"/xe/gun/ion 90 228 0 0" +'\n'+'\n'+'\n')}
        if ISOTOPE_STRING == "U238": {f.write("### U238->Th230 (incl.)" +'\n' +"/xe/gun/ion 92 238 0 0" +'\n' +"/grdm/nucleusLimits 238 230 92 90" +'\n'+'\n')}
        if ISOTOPE_STRING == "Ra226": {f.write("### Ra226" +'\n' +"/xe/gun/ion 88 226 0 0" +'\n'+'\n')}
        if ISOTOPE_STRING == "Th232": {f.write("### Th232->Ac228 (stable)" +'\n' +"/xe/gun/ion 90 232 0 0" +'\n'+"/grdm/nucleusLimits 232 228 90 88"+'\n')}
        #if ISOTOPE_STRING == "U238Pb206": {f.write("### U238->Pb206 (stable)" +'\n' +"/xe/gun/ion 92 238 0 0" +'\n'+ "/grdm/nucleusLimits 238 206 92 80" + '\n' + '\n')}
        if ISOTOPE_STRING == "U235": {f.write("### U235->Pb207 (stable)" +'\n' +"/xe/gun/ion 92 235 0 0" +'\n'+'\n')}
        if ISOTOPE_STRING == "geantinos": {f.write("/xe/gun/energy 0 keV"+ '\n'+ "/xe/gun/particle geantino" + '\n')}
            
        f.write("#ADVANCED RUN OPTIONS" +'\n'  +  "/analysis/settings/setPMTdetails true" + '\n' + "/xe/Postponedecay true" + '\n' + "/run/forced/setVarianceReduction false" +'\n' + "/Xe/detector/setLXeScintillation false" +'\n' + "/run/writeEmpty true" +'\n' + "/Xe/detector/setGdLScintScintillation false")

        f.close()

        os.system("sbatch -o %s/job_%%j.out job.sh %s %s %s %i %s"%(PATH, PATH, MATERIAL_STRING ,ISOTOPE_STRING, EVENT_COUNT, DATE_STRING))
