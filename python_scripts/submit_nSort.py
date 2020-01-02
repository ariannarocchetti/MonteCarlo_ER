#!/usr/bin/python

import sys
import os
from datetime import date	

##### INPUT PARAMETER #####
material_array = ["LXe"]
# Full list of materials: ["LXe", "GXe", "PMT", "Cu", "Ti", "PTFE", "SS", "CryoMat"]
isotope_array = [ "Ra226Pb206", "U238Pb206", "U238Th230"]
# Full list of Isotopes:  ["Co60", "K40", "Cs137", "Ar110m", "Kr85", "Th232Th228", "Ra224Pb208", "U238Th230", "Ra226Pb206", "U235Pb207"]
DATE_STRING = "2018-10-26"


##### ##### #####

for MATERIAL_STRING in material_array:
	for ISOTOPE_STRING in isotope_array:

		PATH ="/sc/userdata/jd1076/DarwinG4_ER_"+ DATE_STRING + "/" + MATERIAL_STRING + "/" + ISOTOPE_STRING 
		os.makedirs(PATH, exist_ok=True)

		os.system("sbatch job_nSort.sh %s %s %s"%(PATH, MATERIAL_STRING ,ISOTOPE_STRING))

