#!/usr/bin/python

import sys
import os
from datetime import date

##### INPUT PARAMETER #####
#material_array = ["LXe"]
material_array = ["SS_OuterCryostat, SS_InnerCryostat, OuterCryostatReflector, SS_BellPlate, SS_BellSideWall, PmtTpc*, Copper_TopRing, Copper_LowerRing, Teflon_Pillar_, Copper_FieldGuard_, Copper_FieldShaperRing_"]


# Full list of materials: ["Ceramic", "Cirlex", "Cu", "Kovar", "PTFE", "Quartz", "SS", "Ti", "Ti_Inner", "Ti_Outer", "SSCryo", "SSPMT"]
isotope_array = ["U238Pb206", "Co60", "K40", "Cs137"]

for MATERIAL_STRING in material_array:
        for ISOTOPE_STRING in isotope_array:

                PATH ="/sc/userdata/arocchetti/XENONnT_" + MATERIAL_STRING + "/" + ISOTOPE_STRING
                os.makedirs(PATH, exist_ok=True)

                os.system("sbatch job_nSort.sh %s %s %s"%(PATH, MATERIAL_STRING ,ISOTOPE_STRING))
