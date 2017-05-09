#! /bin/bash

# change this block to fit your environment
OSPROOT=~/OSPRay
SRCROOT=$(pwd)

# TBB component
export TBB_ROOT=${OSPROOT}/tbb2017_20160916oss
LOAD_TBB()
{
    export LD_LIBRARY_PATH=${TBB_ROOT}/lib/intel64/gcc4.7:${LD_LIBRARY_PATH}
    source ${TBB_ROOT}/bin/tbbvars.sh intel64
}

# ispc component
export ISPC_ROOT=${OSPROOT}/ispc-v1.9.1-linux
LOAD_ISPC()
{
    export PATH=${ISPC_ROOT}:${PATH}
}

# Embree component
export EMBREE_ROOT=${OSPROOT}/embree-2.14.0.x86_64.linux
LOAD_EMBREE()
{
    SET(OSPRAY_USE_EXTERNAL_EMBREE ON) 
    export embree_DIR=${EMBREE_ROOT}
    source ${EMBREE_ROOT}/embree-vars.sh
}

PROJSUFFIX=""
CMAKEARGS=""
CMAKEPATH="cmake"
CMAKEBUILD="./"

HELP()
{
    echo
    echo " This is the OSPRay building helper written by Qi, WU"
    echo
    echo "--- In order to compile the program, you need:"
    echo "---  1) one computer with descent Intel CPU"
    echo "---  2) some required libraries:"
    echo "---     Embree: https://embree.github.io/"
    echo "---     TBB:    https://ispc.github.io/"
    echo "---     ISPC:   https://www.threadingbuildingblocks.org/"
    echo "---     >> download their newest version"
    echo "---        and change pathes in this file"
    echo "Usage build.sh [OPTION]"
    echo
    echo "Description:"
    echo "generate build directory and makefiles for OSPRay"
    echo 
    echo "Option list:"
    echo "  -h, --help          Print help message"
    echo "  -m, --mpi           Build with MPI"
    echo "  -a, --cmake-args    Build with additional cmake arguments"
    echo "  -ic, --intel-icc    Build with intel ICC"
    echo "  -im, --image-magick Build with image magick"
    echo "  -qt, --qt           Build with Qt"
    echo "  --build-prefix      Build prefix" 
    echo "  --embree-dir        Customer Embree path"
    echo "  --tbb-dir           Customer TBB path"
    echo "  --ispc-dir          Customer ISPC path"
    echo "  --cmake-dir         Customer CMAKE path"
    echo "  --no-apps           Disable all OSPRay applications"
    echo 
}

# Stop until all parameters used up
until [ -z "$1" ]; do
    case "$1" in

	# --- Help 
	-h | --help)
 	    HELP
	    exit
	    ;;

	# --- Setup intel icc
	-ic | --intel-icc)
	    source /opt/intel/parallel_studio_xe_2017.0.035/bin/psxevars.sh intel64
	    export INTELICC_PATH=/opt/intel/bin/icc
	    CMAKEARGS=${CMAKEARGS}" -DCMAKE_CXX_COMPILER=${INTELICC_PATH}"
	    CMAKEARGS=${CMAKEARGS}" -DCMAKE_C_COMPILER=${INTELICC_PATH}"
	    shift 1
	    ;;
	    
	# --- Setup mpi flag
	-m  | --mpi)
	    # load information for mpi (need to make sure that we are using intel icc)
	    PROJSUFFIX=${PROJSUFFIX}_mpi
	    CMAKEARGS=${CMAKEARGS}" -DOSPRAY_MODULE_MPI=ON"
	    shift 1
	    ;;

	# --- Setup image magick
	-im | --image-magick)
	    PROJSUFFIX=${PROJSUFFIX}_imagemagick
	    CMAKEARGS=${CMAKEARGS}" -DUSE_IMAGE_MAGICK=ON"
	    shift 1
	    ;;

        # --- Qt component
	-qt | --qt)
	    export QT_ROOT=${DIRROOT}/visitOSPRay/qt-everywhere-opensource-src-4.8.3
	    export PATH=${QT_ROOT}/bin:${PATH}
	    export LD_LIBRARY_PATH=${QT_ROOT}/lib:${LD_LIBRARY_PATH}
	    PROJSUFFIX=${PROJSUFFIX}_qt
	    shift 1
	    ;;

	# --- Setup additionnnnal arguments
	-a  | --cmake-args)
	    CMAKEARGS=${CMAKEARGS}" "${2}; 
	    shift 2
	    ;;

	# --- Setup external libraries
	--embree-dir)
	    export EMBREE_ROOT=${2}
	    echo "user-defined embree path ${EMBREE_ROOT}"
	    shift 2
	    ;;

	--tbb-dir)
	    export TBB_ROOT=${2}
	    echo "user-defined tbb path ${TBB_ROOT}"
	    shift 2
	    ;;

	--ispc-dir)
	    export ISPC_ROOT=${2}
	    echo "user-defined ispc path ${ISPC_ROOT}"
	    shift 2
	    ;;

	--cmake-dir)
	    CMAKEPATH=${2}
	    shift 2
	    ;;

	--build-prefix)
	    CMAKEBUILD=${2}
	    shift 2
	    ;;
	    
	--no-apps)
	    CMAKEARGS=${CMAKEARGS}" -DOSPRAY_APPS_EXAMPLEVIEWER=OFF -DOSPRAY_APPS_PARAVIEW_TFN_CVT=OFF -DOSPRAY_APPS_VOLUMEVIEWER=OFF"
	    shift 1
	    ;;

	# --- error input
	*)
	    HELP
	    exit
	    ;;
    esac
done

LOAD_TBB
LOAD_ISPC
LOAD_EMBREE

# debug
echo "running command: mkdir -p ${CMAKEBUILD}build${PROJSUFFIX}"
echo "running command: cd ${CMAKEBUILD}build${PROJSUFFIX}"
echo "running command: rm -r ./CMakeCache.txt"
echo "running command: ${CMAKEPATH} ${CMAKEARGS} ${SRCROOT}"
echo

# run actual cmake
mkdir -p ${CMAKEBUILD}build${PROJSUFFIX}
cd ${CMAKEBUILD}build${PROJSUFFIX}
rm -r ./CMakeCache.txt
${CMAKEPATH} ${CMAKEARGS} ${SRCROOT}
