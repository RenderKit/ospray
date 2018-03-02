#!/bin/bash
## ======================================================================== ##
## Copyright 2014-2018 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

#### Helper functions ####

# check version of symbols
function check_symbols
{
  for sym in `nm $1 | grep $2_`
  do
    version=(`echo $sym | sed 's/.*@@\(.*\)$/\1/g' | grep -E -o "[0-9]+"`)
    if [ ${#version[@]} -ne 0 ]; then
      if [ ${#version[@]} -eq 1 ]; then version[1]=0; fi
      if [ ${#version[@]} -eq 2 ]; then version[2]=0; fi
      if [ ${version[0]} -gt $3 ]; then
        echo "Error: problematic $2 symbol " $sym
        exit 1
      fi
      if [ ${version[0]} -lt $3 ]; then continue; fi

      if [ ${version[1]} -gt $4 ]; then
        echo "Error: problematic $2 symbol " $sym
        exit 1
      fi
      if [ ${version[1]} -lt $4 ]; then continue; fi

      if [ ${version[2]} -gt $5 ]; then
        echo "Error: problematic $2 symbol " $sym
        exit 1
      fi
    fi
  done
}

#### Set variables for script ####

ROOT_DIR=$PWD
DEP_DIR=$ROOT_DIR/deps

DEP_LOCATION=http://sdvis.org/ospray/download/dependencies/linux
DEP_EMBREE=embree-2.17.1.x86_64.linux
DEP_ISPC=ispc-v1.9.2-linux
DEP_TBB=tbb2018_20170919oss
DEP_TARBALLS="$DEP_EMBREE.tar.gz $DEP_ISPC.tar.gz ${DEP_TBB}_lin.tgz"


# set compiler if the user hasn't explicitly set CC and CXX
if [ -z $CC ]; then
  echo "***NOTE: Defaulting to use icc/icpc!"
  echo -n "         Please set env variables 'CC' and 'CXX' to"
  echo " a different supported compiler (gcc/clang) if desired."
  export CC=icc
  export CXX=icpc
fi

#### Fetch dependencies (TBB+Embree+ISPC) ####

mkdir -p $DEP_DIR
cd $DEP_DIR

for dep in $DEP_TARBALLS ; do
  wget --progress=dot:mega -c $DEP_LOCATION/$dep
  tar -xaf $dep
done
export embree_DIR=$DEP_DIR/$DEP_EMBREE

# fetch docu pdf (in correct version)
BRANCH=$CI_BUILD_REF_NAME
if [ -z $BRANCH ]; then
  BRANCH=`git rev-parse --abbrev-ref HEAD`
fi
wget -N --progress=dot:mega -c http://sdvis.org/ospray/download/OSPRay_readme_$BRANCH.pdf

cd $ROOT_DIR

#### Build OSPRay ####

mkdir -p build_release
cd build_release

# Clean out build directory to be sure we are doing a fresh build
rm -rf *

# CPack expects the docu pdf in the build dir
cp $DEP_DIR/OSPRay_readme_$BRANCH.pdf readme.pdf


# set release and RPM settings
cmake \
-D OSPRAY_BUILD_ISA=ALL \
-D OSPRAY_MODULE_MPI=ON \
-D OSPRAY_MODULE_MPI_APPS=OFF \
-D TBB_ROOT=$DEP_DIR/$DEP_TBB \
-D ISPC_EXECUTABLE=$DEP_DIR/$DEP_ISPC/ispc \
-D OSPRAY_SG_CHOMBO=OFF \
-D OSPRAY_SG_OPENIMAGEIO=OFF \
-D OSPRAY_SG_VTK=OFF \
-D OSPRAY_ZIP_MODE=OFF \
-D OSPRAY_INSTALL_DEPENDENCIES=OFF \
-D CPACK_PACKAGING_INSTALL_PREFIX=/usr \
..

# create RPM files
make -j `nproc` preinstall

check_symbols libospray.so GLIBC   2 4 0
check_symbols libospray.so GLIBCXX 3 4 11
check_symbols libospray.so CXXABI  1 3 0

make -j `nproc` package

# read OSPRay version
OSPRAY_VERSION=`sed -n 's/#define OSPRAY_VERSION "\(.*\)"/\1/p' ospray/version.h`

tar czf ospray-${OSPRAY_VERSION}.x86_64.rpm.tar.gz ospray-*-${OSPRAY_VERSION}-*.rpm

# change settings for zip mode
cmake \
-D OSPRAY_ZIP_MODE=ON \
-D OSPRAY_INSTALL_DEPENDENCIES=ON \
-D CPACK_PACKAGING_INSTALL_PREFIX=/ \
-D CMAKE_INSTALL_INCLUDEDIR=include \
-D CMAKE_INSTALL_LIBDIR=lib \
-D CMAKE_INSTALL_DOCDIR=doc \
-D CMAKE_INSTALL_BINDIR=bin \
..

# create tar.gz files
make -j `nproc` package

