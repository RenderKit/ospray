#!/bin/bash
## ======================================================================== ##
## Copyright 2014-2019 Intel Corporation                                    ##
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
THREADS=`sysctl -n hw.logicalcpu`

# set compiler if the user hasn't explicitly set CC and CXX
if [ -z $CC ]; then
  echo "***NOTE: Defaulting to use clang!"
  echo -n "         Please set env variables 'CC' and 'CXX' to"
  echo " a different supported compiler (gcc/icc) if desired."
  export CC=clang
  export CXX=clang++
fi

# to make sure we do not include nor link against wrong TBB
unset CPATH
unset LIBRARY_PATH
unset DYLD_LIBRARY_PATH

#### Build dependencies ####

mkdir deps_build
cd deps_build

cmake --version

cmake \
  "$@" \
  -D BUILD_JOBS=$THREADS \
  -D BUILD_DEPENDENCIES_ONLY=ON \
  -D CMAKE_INSTALL_PREFIX=$DEP_DIR \
  -D CMAKE_INSTALL_LIBDIR=lib \
  -D BUILD_EMBREE_FROM_SOURCE=OFF \
  -D BUILD_OIDN=ON \
  -D BUILD_OIDN_FROM_SOURCE=OFF \
  -D INSTALL_IN_SEPARATE_DIRECTORIES=OFF \
  ../scripts/superbuild

cmake --build .

cd $ROOT_DIR

#### Build OSPRay ####

mkdir -p build_release
cd build_release

# Clean out build directory to be sure we are doing a fresh build
rm -rf *

# Setup environment variables for dependencies
export OSPCOMMON_TBB_ROOT=$DEP_DIR
export ospcommon_DIR=$DEP_DIR
export embree_DIR=$DEP_DIR
export glfw3_DIR=$DEP_DIR
export openvkl_DIR=$DEP_DIR
export OpenImageDenoise_DIR=$DEP_DIR

# set release and installer settings
cmake -L \
  -D OSPRAY_BUILD_ISA=ALL \
  -D ISPC_EXECUTABLE=$DEP_DIR/bin/ispc \
  -D OSPRAY_ZIP_MODE=OFF \
  -D OSPRAY_MODULE_DENOISER=ON \
  -D OSPRAY_INSTALL_DEPENDENCIES=OFF \
  -D CMAKE_INSTALL_PREFIX=/opt/local \
  -D CMAKE_INSTALL_INCLUDEDIR=include \
  -D CMAKE_INSTALL_LIBDIR=lib \
  -D CMAKE_INSTALL_DOCDIR=../../Applications/OSPRay/doc \
  -D CMAKE_INSTALL_BINDIR=../../Applications/OSPRay/bin \
  ..

# create installers
make -j $THREADS package || exit 2

# change settings for zip mode
cmake -L \
  -D OSPRAY_ZIP_MODE=ON \
  -D OSPRAY_INSTALL_DEPENDENCIES=ON \
  -D CMAKE_INSTALL_INCLUDEDIR=include \
  -D CMAKE_INSTALL_LIBDIR=lib \
  -D CMAKE_INSTALL_DOCDIR=doc \
  -D CMAKE_INSTALL_BINDIR=bin \
  ..

# create ZIP files
make -j $THREADS package || exit 2
