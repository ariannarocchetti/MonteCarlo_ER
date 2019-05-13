#!/usr/bin/env bash
# Arguments
# $1 - job id
# $2 - mc_flavor
# $3 - mc_config
# $4 - events to simulate
# $5 - mc_version
# $6 - preinit_macro
# $7 - source_macro
# $8 - experiment 

function terminate {

    # tar all files                                                                         
    cd ${OUTDIR}
    tar cvjf ${start_dir}/${JOBID}_output.tar.bz2 *
    
    # copy files on stash                                                                    
    #gfal-copy -p file://${G4_FILENAME}.tgz gsiftp://gridftp.grid.uchicago.edu:2811/cephfs/srm/xenon/xenon1t/simulations/mc_$MCVERSION/pax_$PAXVERSION/$MCFLAVOR/$CONFIG/${JOBID}_output.tar.bz2
    
    # Cleanup
    rm -fr $work_dir
    
    cd $start_dir

    exit $1
}


echo "Start time: " `/bin/date`
echo "Job is running on node: " `/bin/hostname`
echo "Job running as user: " `/usr/bin/id`
echo "Job is running in directory: $PWD"

# used to label output
JOBID=$1

# Select MC code flavor
# (G4, NEST, G4p10)
MCFLAVOR=$2

# Specify simulation configuration
# (TPC_Kr83m TPC_Kr85 WholeLXe_Rn220 WholeLXe_Rn222)
CONFIG=$3

# Specify number of events
NEVENTS=$4

# Select MC version
MCVERSION=$5

# Set Experiment
EXPERIMENT=${8}

echo "experiment: $EXPERIMENT" 

# runPatch argument corresponding to CONFIG variable above
if [[ ${CONFIG} == *"Kr83m"* ]]; then
    PATCHTYPE=83
elif [[ ${CONFIG} == *"Kr85"* ]]; then
    PATCHTYPE=85
elif [[ ${CONFIG} == *"Rn220"* ]]; then
    PATCHTYPE=21
elif [[ ${CONFIG} == *"Rn222"* ]]; then
    PATCHTYPE=31
fi
 
start_dir=$PWD


# Setup CVMFS directories
CVMFSDIR=/cvmfs/xenon.opensciencegrid.org
RELEASEDIR=${CVMFSDIR}/releases/mc/${MCVERSION}

# Get the directory where libopcodes is located, LD_LIBRARY_PATH gets wiped
# when source activate is run so we should set it after that for safety
PAX_LIB_DIR=${CVMFSDIR}/releases/anaconda/2.4/envs/pax_${PAXVERSION}/lib/

# Setup Geant4 macros
MACROSDIR=${RELEASEDIR}/macros/${EXPERIMENT}

echo "Macros dir : $MACROSDIR" 


PREINIT_MACRO=${6}
if [[ -z $PREINIT_MACRO ]];
then
    PREINIT_MACRO=preinit_TPC.mac
    if [[ ${CONFIG} == *"muon"* || ${CONFIG} == *"MV"* ]]; then
        PREINIT_MACRO=preinit_MV.mac
    fi
    PREINIT_MACRO=${MACROSDIR}/${PREINIT_MACRO}
else
    if [[ -f ${start_dir}/${PREINIT_MACRO} ]]; then
        PREINIT_MACRO=${start_dir}/${PREINIT_MACRO}
    else
        PREINIT_MACRO=${MACROSDIR}/${PREINIT_MACRO}
    fi
fi
echo "Preinit macro: $PREINIT_MACRO" 

SOURCE_MACRO=${7}
if [[ -z $SOURCE_MACRO ]];
then
    SOURCE_MACRO=${MACROSDIR}/run_${CONFIG}.mac
else
    if [[ -f ${start_dir}/${SOURCE_MACRO} ]]; then
        SOURCE_MACRO=${start_dir}/${SOURCE_MACRO}
    else
        SOURCE_MACRO=${MACROSDIR}/${SOURCE_MACRO}
    fi
fi
echo "Source macro: $SOURCE_MACRO"

# set HOME directory if it's not set
if [[ ${HOME} == "" ]];
then
    export HOME=$PWD
fi

########################################

# Set pipe to propagate error codes to $?
set -o pipefail

# Setup the software
export PATH="${CVMFSDIR}/releases/anaconda/2.4/bin:$PATH"
source activate mc

# make sure libopcodes is in the LD_LIBRARY_PATH
if [[ ! `/bin/env` =~ .*${PAX_LIB_DIR}.* ]];
then
    export LD_LIBRARY_PATH=$PAX_LIB_DIR:$LD_LIBRARY_PATH
fi

if [ $? -ne 0 ];
then
  exit 1
fi

if [[ ${MCFLAVOR} == G4p10 ]]; then
    source ${CVMFSDIR}/software/mc_setup_G4p10.sh
else
    source ${CVMFSDIR}/software/mc_setup_G4p9.sh
fi
if [ $? -ne 0 ];
then
  exit 2
fi

source ${RELEASEDIR}/setup.sh
if [ $? -ne 0 ];
then
  exit 3
fi

# Setting up directories

OUTDIR=$start_dir/output
mkdir -p  ${OUTDIR}
if [ $? -ne 0 ];
then
  exit 4
fi

if [ "$OSG_WN_TMP" == "" ];
then
    OSG_WN_TMP=$PWD
fi

work_dir=`mktemp -d --tmpdir=$OSG_WN_TMP`
cd $work_dir

# Filenaming
SUBRUN=`printf "%05d\n" $JOBID`
FILEROOT=Xenon1T_${CONFIG}
FILENUM=${FILEROOT}_${SUBRUN}
FILENAME=${OUTDIR}/${FILENUM}
G4_FILENAME=${FILENAME}_g4mc_${MCFLAVOR}
G4PATCH_FILENAME=${G4_FILENAME}_Patch
G4NSORT_FILENAME=${G4_FILENAME}_Sort

# Start of simulations #

# Geant4 stage
G4EXEC=${RELEASEDIR}/xenon1t_${MCFLAVOR}
SPECTRADIR=${RELEASEDIR}/macros
ln -sf ${SPECTRADIR} # For reading e.g. input spectra from CWD

(time ${G4EXEC} -p ${PREINIT_MACRO} -f ${SOURCE_MACRO} -n ${NEVENTS} -d ${EXPERIMENT} -o ${G4_FILENAME}.root;) 2>&1 | tee ${G4_FILENAME}.log
if [ $? -ne 0 ];
then
    terminate 10
fi

# Skip the rest for optical photons
if [[ ${CONFIG} == *"optPhot"* ]]; then
    terminate 0
fi

# Skip the rest for XENONnT, will move along as XENONnT chain takes shape
#if [[ ${EXPERIMENT} == "XENONnT" ]]; then
#    terminate 0
#fi

CPATH=${OLD_CPATH}
source ${CVMFSDIR}/software/mc_setup_G4p9.sh

# nSort Stage
ln -sf ${RELEASEDIR}/data

echo "------------------------ACTIVATING nSort--------------------------------------" 

    
# Old nSort executable
NSORTEXEC=${RELEASEDIR}/nSort
#NSORTEXEC=/home/rocchetti/MonteCarlo-studies-/nSort

#cd nSort/
#./nSort /scratch/$outfile 2 1 0 0 0


#next three lines original 
(time ${NSORTEXEC} -i ${G4_FILENAME} -d ${EXPERIMENT} 2 1 0 0 0;) 2>&1 | tee ${G4NSORT_FILENAME}.log
echo "---------> experiment is : $EXPERIMENT"
echo "---------> nSort activated in :$NSORTEXEC"
    
# XENON1T SR0 models
ln -sf ${RELEASEDIR}/nSort/* .
source deactivate
CPATH=${OLD_CPATH}
#rm -r ~/.cache/rootpy/*
#only for 1t
#source activate pax_head
#python GenerateGeant4.py --InputFile ${G4_FILENAME}.root --OutputFilename ${G4NSORT_FILENAME}.root
   
#if [ $? -ne 0 ];
#then
#    terminate 12
#fi
#    PAX_INPUT_FILENAME=${G4NSORT_FILENAME}

 
# Move hax output
#cp *.root *.pklz ${OUTDIR} 

#rm ${PAX_FILENAME}.root  # Delete pax output for now

terminate 0
