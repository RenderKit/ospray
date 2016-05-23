## ======================================================================== ##
## Copyright 2014-2016 Intel Corporation                                    ##
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

#!/bin/bash

#### Helper functions ####

# check version of symbols
function check_symbols
{
  for sym in `nm $1 | grep $2_`
  do
    version=(`echo $sym | sed 's/.*@@\(.*\)$/\1/p' | grep -E -o "[0-9]+"`)
    if [ ${#version[@]} -ne 0 ]; then
      #echo "version0 = " ${version[0]}
      #echo "version1 = " ${version[1]}
      if [ ${version[0]} -gt $3 ]; then
        echo "Error: problematic $2 symbol " $sym
        exit 1
      fi
      if [ ${version[1]} -gt $4 ]; then
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
DEP_EMBREE=embree_dynamic-2.9.0.x86_64.linux
DEP_ISPC=ispc-v1.9.0-linux
DEP_TBB=tbb44_20160413oss
DEP_TARBALLS="$DEP_EMBREE.tar.gz $DEP_ISPC.tar.gz ${DEP_TBB}_lin.tgz"


# set compiler if the user hasn't explicitly set CC and CXX
if [ -z $CC ]; then
  echo "***NOTE: Defaulting to use icc/icpc!"
  echo -n "         Please set env variables 'CC' and 'CXX' to"
  echo " a different supported compiler (gcc/clang) if desired."
  export CC=icc
  export CXX=icpc
fi

# to make sure we do not include nor link against wrong TBB
# NOTE: if we are not verifying CentOS6 defaults, we are likely using
#       a different compiler which requires LD_LIBRARY_PATH!
if [ -n $OSPRAY_RELEASE_NO_VERIFY ]; then
  unset CPATH
  unset LIBRARY_PATH
  unset LD_LIBRARY_PATH
fi

#### Fetch dependencies (TBB+Embree+ISPC) ####

mkdir -p $DEP_DIR
cd $DEP_DIR

for dep in $DEP_TARBALLS ; do
  wget --progress=dot:mega -c $DEP_LOCATION/$dep
  tar -xaf $dep
done
export embree_DIR=$DEP_DIR/$DEP_EMBREE

cd $ROOT_DIR

#### Build OSPRay ####

mkdir -p build_release
cd build_release

# Clean out build directory to be sure we are doing a fresh build
rm -rf *


# set release and RPM settings
cmake \
-D OSPRAY_BUILD_ISA=ALL \
-D OSPRAY_BUILD_ENABLE_KNL=ON \
-D OSPRAY_BUILD_MIC_SUPPORT=OFF \
-D OSPRAY_BUILD_COI_DEVICE=OFF \
-D OSPRAY_BUILD_MPI_DEVICE=OFF \
-D OSPRAY_USE_EXTERNAL_EMBREE=ON \
-D TBB_ROOT=$DEP_DIR/$DEP_TBB \
-D ISPC_EXECUTABLE=$DEP_DIR/$DEP_ISPC/ispc \
-D USE_IMAGE_MAGICK=OFF \
-D OSPRAY_ZIP_MODE=OFF \
-D CMAKE_INSTALL_PREFIX=/usr \
..

# create RPM files
make -j `nproc` preinstall

# if we define 'OSPRAY_RELEASE_NO_VERIFY' to anything, then we
#   don't verify link dependencies for CentOS6
if [ -z $OSPRAY_RELEASE_NO_VERIFY ]; then
  check_symbols libospray.so GLIBC   2 4
  check_symbols libospray.so GLIBCXX 3 4
  check_symbols libospray.so CXXABI  1 3
fi

make -j `nproc` package

# read OSPRay version
OSPRAY_VERSION=`sed -n 's/#define OSPRAY_VERSION "\(.*\)"/\1/p' ospray/version.h`

# rename RPMs to have component name before version
for i in ospray-${OSPRAY_VERSION}-1.*.rpm ; do 
  newname=`echo $i | sed -e "s/ospray-\(.\+\)-\([a-z_]\+\)\.rpm/ospray-\2-\1.rpm/"`
  mv $i $newname
done

tar czf ospray-${OSPRAY_VERSION}.x86_64.rpm.tar.gz ospray-*-${OSPRAY_VERSION}-1.x86_64.rpm

# change settings for zip mode
cmake \
-D OSPRAY_ZIP_MODE=ON \
-D CMAKE_INSTALL_INCLUDEDIR=include \
-D CMAKE_INSTALL_LIBDIR=lib \
-D CMAKE_INSTALL_DOCDIR=doc \
-D CMAKE_INSTALL_BINDIR=bin \
..

# create tar.gz files
make -j `nproc` package

