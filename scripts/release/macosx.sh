#!/bin/bash
## Copyright 2014 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

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
DEP_BUILD_DIR=$ROOT_DIR/build_deps
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

mkdir $DEP_BUILD_DIR
cd $DEP_BUILD_DIR

cmake --version

cmake \
  "$@" \
  -D BUILD_JOBS=$THREADS \
  -D BUILD_DEPENDENCIES_ONLY=ON \
  -D CMAKE_INSTALL_PREFIX=$DEP_DIR \
  -D CMAKE_INSTALL_LIBDIR=lib \
  -D INSTALL_IN_SEPARATE_DIRECTORIES=OFF \
  ../scripts/superbuild

cmake --build .

cd $ROOT_DIR

#### Build OSPRay ####

mkdir -p build_release
cd build_release

# Clean out build directory to be sure we are doing a fresh build
rm -rf *

# Setup environment for dependencies
export CMAKE_PREFIX_PATH=$DEP_DIR

# set release and installer settings
cmake -L \
  "$@" \
  -D OSPRAY_BUILD_ISA=ALL \
  -D TBB_ROOT=$DEP_DIR \
  -D OSPRAY_ZIP_MODE=ON \
  -D OSPRAY_MODULE_DENOISER=ON \
  -D OSPRAY_INSTALL_DEPENDENCIES=ON \
  -D CMAKE_INSTALL_INCLUDEDIR=include \
  -D CMAKE_INSTALL_LIBDIR=lib \
  -D CMAKE_INSTALL_DOCDIR=doc \
  -D CMAKE_INSTALL_BINDIR=bin \
  -D CMAKE_BUILD_WITH_INSTALL_RPATH=ON \
  -D CPACK_PRE_BUILD_SCRIPTS=$CPACK_PRE_BUILD_SIGNING_SCRIPT \
  ..

make -j $THREADS package || exit 2
