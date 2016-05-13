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

umask=`umask`
function onexit {
  umask $umask
}
trap onexit EXIT
umask 002

#### Set variables for script ####

export ROOT_DIR=$PWD

DEP_LOCATION=http://sdvis.org/~jdamstut/ospray_deps/osx
TBB_TARBALL=embree-2.9.0.x86_64.osx.tar.gz
EMBREE_TARBALL=tbb44_20160413oss_osx.tgz
ISPC_TARBALL=ispc-v1.9.0-osx.tar.gz

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

if [ ! -d deps ]; then
  mkdir deps
  rm -rf deps/*
  cd deps

  # TBB
  wget $DEP_LOCATION/$TBB_TARBALL
  tar -xaf $TBB_TARBALL
  rm $TBB_TARBALL

  # Embree
  wget $DEP_LOCATION/$EMBREE_TARBALL
  tar -xaf $EMBREE_TARBALL
  rm $EMBREE_TARBALL

  # ISPC
  wget $DEP_LOCATION/$ISPC_TARBALL
  tar -xaf $ISPC_TARBALL
  rm $ISPC_TARBALL

  cd $ROOT_DIR
  ln -snf deps/tbb* tbb
  ln -snf deps/embree* embree
  ln -snf deps/ispc* ispc
fi

TBB_PATH_LOCAL=$ROOT_DIR/tbb
export embree_DIR=$ROOT_DIR/embree
export PATH=$ROOT_DIR/ispc:$PATH

mkdir -p build_release
cd build_release
# make sure to use default settings
rm -f CMakeCache.txt
rm -f ospray/version.h

# set release and installer settings
cmake \
-D OSPRAY_BUILD_ISA=ALL \
-D OSPRAY_BUILD_MIC_SUPPORT=OFF \
-D OSPRAY_BUILD_COI_DEVICE=OFF \
-D OSPRAY_BUILD_MPI_DEVICE=OFF \
-D USE_IMAGE_MAGICK=OFF \
-D OSPRAY_ZIP_MODE=OFF \
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
-D CMAKE_INSTALL_INCLUDEDIR=include \
-D CMAKE_INSTALL_LIBDIR=lib \
-D CMAKE_INSTALL_DOCDIR=doc \
-D CMAKE_INSTALL_BINDIR=bin \
-D TBB_ROOT=$TBB_PATH_LOCAL \
..

# create ZIP files
make -j 4 package

