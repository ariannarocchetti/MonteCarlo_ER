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



DATE_STRING = str(date.today())
#DATE_STRING = "2019-11-04"
for MATERIAL_STRING in material_array:
    for ISOTOPE_STRING in isotope_array:

        PATH ="/sc/userdata/arocchetti/XENONnT_" + DATE_STRING + "/" + MATERIAL_STRING + "/" + ISOTOPE_STRING
        os.makedirs(PATH, exist_ok=True)

        print(PATH)
        print("job_nSort.sh", PATH, MATERIAL_STRING ,ISOTOPE_STRING)
        os.system("sbatch -o %s/job_%%j_Sort.out job_nSort.sh %s %s %s"%(PATH, PATH, MATERIAL_STRING ,ISOTOPE_STRING))
