#!/bin/bash
## Copyright 2014-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

#### Helper functions ####

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

function check_imf
{
for lib in "$@"
do
  if [ -n "`ldd $lib | fgrep libimf.so`" ]; then
    echo "Error: dependency to 'libimf.so' found"
    exit 3
  fi
done
}

#### Set variables for script ####

ROOT_DIR=$PWD
DEP_DIR=$ROOT_DIR/deps
DEP_BUILD_DIR=$ROOT_DIR/build_deps
THREADS=`nproc`


#### Build dependencies ####

mkdir $DEP_BUILD_DIR
cd $DEP_BUILD_DIR

# NOTE(jda) - Some Linux OSs need to have lib/ on LD_LIBRARY_PATH at build time
export LD_LIBRARY_PATH=$DEP_DIR/lib:${LD_LIBRARY_PATH}

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
export rkcommon_DIR=$DEP_DIR
export embree_DIR=$DEP_DIR
export glfw3_DIR=$DEP_DIR
export openvkl_DIR=$DEP_DIR
export OpenImageDenoise_DIR=$DEP_DIR

# set release settings
cmake -L \
  -D OSPRAY_BUILD_ISA=ALL \
  -D TBB_ROOT=$DEP_DIR \
  -D ISPC_EXECUTABLE=$DEP_DIR/bin/ispc \
  -D OSPRAY_ZIP_MODE=ON \
  -D OSPRAY_MODULE_DENOISER=ON \
  -D OSPRAY_INSTALL_DEPENDENCIES=ON \
  -D CMAKE_INSTALL_INCLUDEDIR=include \
  -D CMAKE_INSTALL_LIBDIR=lib \
  -D CMAKE_INSTALL_DOCDIR=doc \
  -D CMAKE_INSTALL_BINDIR=bin \
  -D CMAKE_BUILD_WITH_INSTALL_RPATH=ON \
  ..

# create tar.gz
make -j $THREADS preinstall

# verify libs
check_symbols libospray.so GLIBC   2 17 0
check_symbols libospray.so GLIBCXX 3 4 19
check_symbols libospray.so CXXABI  1 3 7

check_symbols libospray_module_ispc.so GLIBC   2 17 0
check_symbols libospray_module_ispc.so GLIBCXX 3 4 19
check_symbols libospray_module_ispc.so CXXABI  1 3 7

check_imf libospray.so
check_imf libospray_module_ispc.so

make -j $THREADS package || exit 2
