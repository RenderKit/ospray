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

umask=`umask`
function onexit {
  umask $umask
}
trap onexit EXIT
umask 002

#### Set variables for script ####

ROOT_DIR=$PWD
DEP_DIR=$ROOT_DIR/deps

DEP_LOCATION=http://sdvis.org/ospray/download/dependencies/osx
DEP_EMBREE=embree-2.17.1.x86_64.macosx
DEP_ISPC=ispc-v1.9.2-osx
DEP_TBB=tbb2018_20170919oss
DEP_TARBALLS="$DEP_EMBREE.tar.gz $DEP_ISPC.tar.gz ${DEP_TBB}_mac.tgz"


# set compiler if the user hasn't explicitly set CC and CXX
if [ -z $CC ]; then
  echo "***NOTE: Defaulting to use clang!"
  echo -n "         Please set env variables 'CC' and 'CXX' to"
  echo " a different supported compiler (gcc/icc) if desired."
  export CC=clang
  export CXX=clang++
fi

# to make sure we do not include nor link against wrong TBB
export CPATH=
export LIBRARY_PATH=
export DYLD_LIBRARY_PATH=
TBB_PATH_LOCAL=$PWD/tbb

#### Fetch dependencies (TBB+Embree+ISPC) ####

mkdir -p $DEP_DIR
cd $DEP_DIR

for dep in $DEP_TARBALLS ; do
  wget --progress=dot:mega -c $DEP_LOCATION/$dep
  tar -xf $dep
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


# set release and installer settings
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
-D CMAKE_INSTALL_PREFIX=/opt/local \
-D CMAKE_INSTALL_INCLUDEDIR=include \
-D CMAKE_INSTALL_LIBDIR=lib \
-D CMAKE_INSTALL_DOCDIR=../../Applications/OSPRay/doc \
-D CMAKE_INSTALL_BINDIR=../../Applications/OSPRay/bin \
..

# create installers
make -j 4 package

# change settings for zip mode
cmake \
-D OSPRAY_ZIP_MODE=ON \
-D OSPRAY_INSTALL_DEPENDENCIES=ON \
-D CMAKE_INSTALL_INCLUDEDIR=include \
-D CMAKE_INSTALL_LIBDIR=lib \
-D CMAKE_INSTALL_DOCDIR=doc \
-D CMAKE_INSTALL_BINDIR=bin \
..

# create ZIP files
make -j 4 package

