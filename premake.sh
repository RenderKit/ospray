#! /bin/bash

if [[ "$(hostname)" != "hastur.sci.utah.edu" ]]; then
    echo "Warning: You are suggested to change "
fi

# initialize compiler
# ICC
USE_ICC=0
ICC_PATH=$(which icc)
# GCC
USE_GCC=0
GCC_PATH=$(which gcc)
GXX_PATH=$(which g++)

# initialization
CUSTOMIZED_PROJSUFFIX=""
PROJSUFFIX=""
PROJPREFIX="./"
CMAKEARGS=" -DOSPRAY_MODULE_VISIT=ON" # enable visit module by default XD
CMAKEPATH="cmake"

#-------------------------------------------------------------------------------
# change this block to fit your environment
OSPROOT=~/software # the directory for all dependencies
SRCROOT=$(pwd)

# TBB component
TBB_ROOT=${OSPROOT}/tbb2017_20170604oss
LOAD_TBB()
{
    source ${TBB_ROOT}/bin/tbbvars.sh intel64
    CMAKEARGS=${CMAKEARGS}" -DTBB_ROOT="${TBB_ROOT}
}

# ispc component
ISPC_ROOT=${OSPROOT}/ispc-v1.9.1-linux
LOAD_ISPC()
{
    CMAKEARGS=${CMAKEARGS}" -DISPC_DIR_HINT="${ISPC_ROOT}
}

# Embree component
EMBREE_ROOT=${OSPROOT}/embree-2.15.0.x86_64.linux
LOAD_EMBREE()
{
    source ${EMBREE_ROOT}/embree-vars.sh
    CMAKEARGS=${CMAKEARGS}" -Dembree_DIR="${EMBREE_ROOT}
}

# QT path
QT_PATH=${OSPROOT}/qt-4.8.6/qt-everywhere-opensource-src-4.8.6-install
#-------------------------------------------------------------------------------

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
    echo "  -h , --help         Print help message"
    echo "  -i , --prefix       Build with customized install path"
    echo "  -a , --cmake-args   Build with additional cmake arguments"
    echo "  -o , --output-dir   Build directory name"
    echo "    (If not set, build directory will be generated based on rules)"
    echo "  --build-prefix      Build directory prefix"
    echo "    (Build directory will be generated based on rules)"
    echo "  --mpi               Build with MPI"
    echo "  --icc               Build with intel ICC"
    echo "  --gcc               Build with intel GCC"
    echo "  --qt                Build with Qt"
    echo "  --image-magick      Build with image magick"
    echo "  --embree-dir        Customized Embree path"
    echo "  --tbb-dir           Customized TBB path"
    echo "  --ispc-dir          Customized ISPC path"
    echo "  --icc-dir           Customized ICC path"
    echo "  --gcc-dir           Customized GCC binary path"
    echo "  --cmake-dir         Customized CMAKE path"
    echo "  --no-apps           Disable all OSPRay applications"
    echo "    (Turn off all ospray applications)"
    echo 
}

# Stop until all parameters used up
ARGCACHE=""
if [ -z "$1" ]; then
    HELP
    exit
fi
until [ -z "$1" ]; do
    case "$1" in

	# --- Help 
	-h | --help)
 	    HELP
	    exit
	    ;;

	# --- Setup additionnnnal arguments
	-a  | --cmake-args)
	    CMAKEARGS=${CMAKEARGS}" "${2}
	    ARGCACHE=${ARGCACHE}" -a "'"'${2}'"'
	    shift 2
	    ;;

	# --- setup customized project suffix
	-o | --output-dir)
	    CUSTOMIZED_PROJSUFFIX=${2}
	    ARGCACHE=${ARGCACHE}" -o "${2}
	    shift 2
	    ;;

	# --- setup customized install prefix
	-i | --install-dir | --prefix | --install-prefix)
	    INSTALL_PREFIX=${2}
	    if [[ "${INSTALL_PREFIX}" != /* ]]; then
		if [[ "${INSTALL_PREFIX}" != ~/* ]]; then
		    INSTALL_PREFIX=$(pwd)/${INSTALL_PREFIX}
		fi
	    fi
	    CMAKEARGS=${CMAKEARGS}" -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}
	    ARGCACHE=${ARGCACHE}" -i "${INSTALL_PREFIX}
	    shift 2
	    ;;

	# --- Setup compiler
	--gcc)
	    USE_GCC=1
	    USE_ICC=0
	    ARGCACHE=${ARGCACHE}" --gcc"
	    shift 1
	    ;;

	--icc)
	    USE_ICC=1
	    USE_GCC=0
	    ARGCACHE=${ARGCACHE}" --icc"
	    shift 1
	    ;;
	    
	# --- Setup mpi flag
	--mpi)
	    # load information for mpi (need to make sure that we are using intel icc)
	    PROJSUFFIX=${PROJSUFFIX}_mpi
	    CMAKEARGS=${CMAKEARGS}" -DOSPRAY_MODULE_MPI=ON"
	    ARGCACHE=${ARGCACHE}" --mpi"
	    shift 1
	    ;;

	# --- Setup image magick
	--image-magick)
	    PROJSUFFIX=${PROJSUFFIX}_imagemagick
	    CMAKEARGS=${CMAKEARGS}" -DUSE_IMAGE_MAGICK=ON"
	    ARGCACHE=${ARGCACHE}" --image-magick"
	    shift 1
	    ;;

        # --- Qt component
	--qt)
	    export QT_ROOT=${QT_PATH}
	    export PATH=${QT_ROOT}/bin:${PATH}
	    export LD_LIBRARY_PATH=${QT_ROOT}/lib:${LD_LIBRARY_PATH}
	    PROJSUFFIX=${PROJSUFFIX}_qt
	    ARGCACHE=${ARGCACHE}" --qt"
	    shift 1
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

	--icc-dir)
	    ICC_PATH=${2}/icc
	    echo "user-defined icc path ${ICC_PATH}"
	    shift 2
	    ;;

	--gcc-dir)
	    GCC_PATH=${2}/gcc
	    GXX_PATH=${2}/g++
	    echo "user-defined gcc path ${GCC_PATH} ${G++_PATH}"
	    shift 2
	    ;;

	--cmake-dir)
	    CMAKEPATH=${2}
	    shift 2
	    ;;

	--build-prefix)
	    PROJPREFIX=${2}
	    ARGCACHE=${ARGCACHE}" --build-prefix "${2}
	    shift 2
	    ;;
	    
	--no-apps)
	    CMAKEARGS=${CMAKEARGS}" -DOSPRAY_ENABLE_APPS=OFF -DOSPRAY_MODULE_VISIT_APPS=OFF"
	    ARGCACHE=${ARGCACHE}" --no-apps"
	    shift 1
	    ;;

	# --- error input
	*)
	    echo "wrong argument: "${1}
	    HELP
	    exit
	    ;;
    esac
done

# setup compiler
if [[ "${USE_GCC}" == "1" ]]; then
    CMAKEARGS=${CMAKEARGS}" -DCMAKE_CXX_COMPILER=${GXX_PATH}"
    CMAKEARGS=${CMAKEARGS}" -DCMAKE_C_COMPILER=${GCC_PATH}"    
fi
if [[ "${USE_ICC}" == "1" ]]; then
    CMAKEARGS=${CMAKEARGS}" -DCMAKE_CXX_COMPILER=${ICC_PATH}"
    CMAKEARGS=${CMAKEARGS}" -DCMAKE_C_COMPILER=${ICC_PATH}"
fi

# load libraries
LOAD_TBB
LOAD_ISPC
LOAD_EMBREE
if [[ "${CUSTOMIZED_PROJSUFFIX}" == "" ]]; then
    PROJDIR=${PROJPREFIX}build${PROJSUFFIX}
else
    PROJDIR=${CUSTOMIZED_PROJSUFFIX}
fi

# debug
echo "running command: mkdir -p ${PROJDIR}"
echo "running command: cd ${PROJDIR}"
echo "running command: rm -r ./CMakeCache.txt"
echo "running command: ${CMAKEPATH} ${CMAKEARGS} ${SRCROOT}"
echo

# run actual cmake
mkdir -p ${PROJDIR}
cd ${PROJDIR}
rm -r ./CMakeCache.txt ./CMakeFiles
${CMAKEPATH} ${CMAKEARGS} ${SRCROOT}
cd -

# save cache
cat > premake.local.$(hostname).sh <<EOF
#!/bin/bash
$(pwd)/premake.sh ${ARGCACHE} --embree-dir "${EMBREE_ROOT}" --tbb-dir "${TBB_ROOT}" --ispc-dir "${ISPC_ROOT}" --icc-dir "${ICC_PATH%/*}" --gcc-dir "${GCC_PATH%/*}" --cmake-dir "${CMAKEPATH}"
EOF
