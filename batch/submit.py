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
EVENT_COUNT = 100
#POSTPONE_DECAY = ["true"]
DATE_STRING = str(date.today())

##### ##### #####

for MATERIAL_STRING in material_array:
    for ISOTOPE_STRING in isotope_array:

        PATH ="/sc/userdata/arocchetti/XENONnT_"+ DATE_STRING + "/" + MATERIAL_STRING + "/" + ISOTOPE_STRING
        os.makedirs(PATH, exist_ok=True)

        MACRONAME = PATH + "/" + "run_ER_" + MATERIAL_STRING + "_" + ISOTOPE_STRING + ".mac"

        f = open(MACRONAME, "w")

        f.write("#VERBOSITY" +'\n' +"/control/verbose 0" +'\n' + "/run/verbose 0" +'\n' +"/event/verbose 0" +'\n' +"/tracking/verbose 0" +'\n' + "/xe/gun/verbose 0" +'\n' +'\n')
        f.write("#SEED" +'\n' "/run/random/setRandomSeed 0" +'\n' +'\n')
        f.write("# General source settings"  +'\n' +"/xe/gun/angtype  iso" +'\n' +"/xe/gun/type  Volume" +'\n' +"/xe/gun/shape  Cylinder" +'\n' +"/xe/gun/center  0. 0. -70. cm" +'\n' + "/xe/gun/radius 100. cm" + '\n' + "/xe/gun/halfz 170. cm" + '\n' + "/xe/gun/energy 0 keV"+ '\n' + "/xe/gun/particle ion" + '\n' + '\n')

        f.write("/xe/gun/confine " + MATERIAL_STRING + "*" + '\n')

        if ISOTOPE_STRING == "Co60": {f.write("### Co60" +'\n' +"/xe/gun/ion 27 60 0 0" +'\n'+'\n')}
        if ISOTOPE_STRING == "K40": {f.write("### K40" +'\n' +"/xe/gun/ion 19 40 0 0" +'\n'+'\n')}
        if ISOTOPE_STRING == "Cs137": {f.write("### Cs137" +'\n' +"/xe/gun/ion 55 137 0 0" +'\n'+'\n')}
        #if ISOTOPE_STRING == "Ag110m": {f.write("### Ag110m" +'\n' +"/darwin/gun/ion 47 110 0 117.59" +'\n'+'\n')}
        #if ISOTOPE_STRING == "Rb83Kr83": {f.write("### Rb83->Kr83 (stable)" +'\n' +"/darwin/gun/ion 36 83 0 0 " +'\n' +"/grdm/nucleusLimits 83 83 37 36"  +'\n'+'\n')}
        #if ISOTOPE_STRING == "Kr85": {f.write("### Kr85" +'\n' +"/darwin/gun/ion 36 85 0 0 " +'\n'+'\n')}
	
        #splitted chain        
        #if ISOTOPE_STRING == "Th232Th228": {f.write("### Th232->Th228 (incl.)" +'\n' +"/darwin/gun/ion 90 232 0 0" +'\n' +"/grdm/nucleusLimits 232 228 90 88" +'\n'+'\n')}
        #if ISOTOPE_STRING == "Th228Pb208": {f.write("### Th228->Pb208 (stable)" +'\n' +"/darwin/gun/ion 90 228 0 0" +'\n' +"/grdm/nucleusLimits 228 208 90 81" +'\n'+'\n')}
        #if ISOTOPE_STRING == "U238Th230": {f.write("### U238->Th230 (incl.)" +'\n' +"/darwin/gun/ion 92 238 0 0" +'\n' +"/grdm/nucleusLimits 238 230 92 90" +'\n'+'\n')}
        #if ISOTOPE_STRING == "Th230Pb206": {f.write("### Th230->Pb206 (stable)" +'\n' +"/darwin/gun/ion 90 230 0 0" +'\n' +"/grdm/nucleusLimits 230 206 90 82" +'\n'+'\n')}
        
        if ISOTOPE_STRING == "Th232Pb208": {f.write("### Th232->Pb208 (stable)" +'\n' +"/xe/gun/ion 90 232 0 0" +'\n'+'\n')}
        if ISOTOPE_STRING == "U238Pb206": {f.write("### U238->Pb206 (stable)" +'\n' +"/xe/gun/ion 92 238 0 0" +'\n'+ "/grdm/nucleusLimits 238 206 92 80" + '\n' + '\n')}
        if ISOTOPE_STRING == "U235Pb207": {f.write("### U235->Pb207 (stable)" +'\n' +"/xe/gun/ion 92 235 0 0" +'\n'+'\n')}
        #if ISOTOPE_STRING == "Ti44Sc44Ca44": {f.write("### Ti44->Ca44 (stable)" + '\n' + "/darwin/gun/ion 22 44 0 0" + '\n' + '\n')}
        #if ISOTOPE_STRING == "Tl208": {f.write("### Tl208->Pb208 (stable)" + '\n' + "/darwin/gun/ion 81 208 0 0" + '\n' + "/grdm/nucleusLimits 208 208 81 81" + '\n')}
        #if ISOTOPE_STRING == "Bi214": {f.write("### Bi214->Po214 (unstable)" + '\n' + "/darwin/gun/ion 83 214 0 0" + '\n' + "/grdm/nucleusLimits 214 214 83 83" + '\n')}
        #if ISOTOPE_STRING == "Sc44": {f.write("### Sc44->Ca44 (stable)" + '\n' + "/darwin/gun/ion 20 44 0 2656.53" + '\n' )}

        f.write("#ADVANCED RUN OPTIONS" +'\n' + "/analysis/settings/setPMTdetails true" + '\n' + "/xe/Postponedecay true" + '\n' + "/run/forced/setVarianceReduction false" +'\n' + "/Xe/detector/setLXeScintillation false" +'\n' + "/run/writeEmpty true" +'\n' + "/Xe/detector/setGdLScintScintillation false")

        f.close()

        os.system("sbatch -o %s/job_%%j.out job.sh %s %s %s %i %s"%(PATH, PATH, MATERIAL_STRING ,ISOTOPE_STRING, EVENT_COUNT, DATE_STRING))
