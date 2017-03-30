#! /bin/bash

# change this block to fit your environment
DIRROOT=~
OSPROOT=~/OSPRay
export QT_ROOT=${DIRROOT}/visitOSPRay/qt-everywhere-opensource-src-4.8.3
export TBB_ROOT=${OSPROOT}/tbb2017_20160916oss
export ISPC_ROOT=${OSPROOT}/ispc-v1.9.1-linux
export EMBREE_ROOT=${OSPROOT}/embree-2.14.0.x86_64.linux
export INTELICC_PATH=/opt/intel/bin/icc
source /opt/intel/parallel_studio_xe_2017.0.035/bin/psxevars.sh intel64

# Qt component
export PATH=${QT_ROOT}/bin:${PATH}
export LD_LIBRARY_PATH=${QT_ROOT}/lib:${LD_LIBRARY_PATH}

# TBB component
source ${TBB_ROOT}/bin/tbbvars.sh intel64
export LD_LIBRARY_PATH=${TBB_ROOT}/lib/intel64/gcc4.7:${LD_LIBRARY_PATH}

# ispc component
export PATH=${ISPC_ROOT}:${PATH}

PROJSUFFIX=""
CMAKEARGS=""

# Stop until all parameters used up
until [ -z "$1" ]; do
    case "$1" in
	# --- Help 
	-h | --help) 	    
	    echo "--- This is the OSPRay building helper written by Qi"
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
	    echo "  -e, --embree        Build with external Embree"
	    echo "  -a, --cmake-args    Build with additional cmake arguments"
	    echo "  -ic, --intel-icc    Build with intel ICC"
	    echo "  -im, --image-magick Build with image magick"
	    echo 
	    exit
	    ;;

	# --- Setup intel icc
	-ic | --intel-icc)
	    CMAKEARGS=${CMAKEARGS}" -DCMAKE_CXX_COMPILER=${INTELICC_PATH}"
	    CMAKEARGS=${CMAKEARGS}" -DCMAKE_C_COMPILER=${INTELICC_PATH}"
	    shift 1;;
	    
	# --- Setup mpi flag
	-m  | --mpi)
	    # load information for mpi (need to make sure that we are using intel icc)
	    PROJSUFFIX=${PROJSUFFIX}_mpi
	    CMAKEARGS=${CMAKEARGS}" -DOSPRAY_MODULE_MPI=ON"
	    shift 1;;

	# --- Setup external embree
	-e  | --embree)
	    export embree_DIR=${EMBREE_ROOT}
	    PROJSUFFIX=${PROJSUFFIX}_embree
	    CMAKEARGS=${CMAKEARGS}" -DOSPRAY_USE_EXTERNAL_EMBREE=ON"
	    shift 1;;

	# --- Setup image magick
	-im | --image-magick)
	    PROJSUFFIX=${PROJSUFFIX}_imagemagick
	    CMAKEARGS=${CMAKEARGS}" -DUSE_IMAGE_MAGICK=ON"
	    shift 1;;

	# --- Setup additionnnnal arguments
	-a  | --cmake-args)
	    CMAKEARGS=${CMAKEARGS}" "${2}; shift 2;;

    esac
done

# debug
echo "running command: mkdir -p build${PROJSUFFIX}"
echo "running command: cd build${PROJSUFFIX}"
echo "running command: rm -r ./*"
echo "running command: cmake ${CMAKEARGS} .."
# run actual cmake
mkdir -p build${PROJSUFFIX}
cd build${PROJSUFFIX}
rm -r ./*
cmake ${CMAKEARGS} ..
