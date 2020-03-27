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
OSPRAY_PKG_BASE=ospray-2.1.0.x86_64.linux
OSPRAY_BUILD_DIR=$ROOT_DIR/build_release
INSTALL_DIR=$ROOT_DIR/install/$OSPRAY_PKG_BASE
THREADS=`nproc`

#### Cleanup any existing directories ####

rm -rf $DEP_DIR
rm -rf $DEP_BUILD_DIR
rm -rf $OSPRAY_BUILD_DIR
rm -rf $INSTALL_DIR

#### Build dependencies ####

mkdir $DEP_BUILD_DIR
cd $DEP_BUILD_DIR

# NOTE(jda) - Some Linux OSs need to have lib/ on LD_LIBRARY_PATH at build time
export LD_LIBRARY_PATH=$DEP_DIR/lib:${LD_LIBRARY_PATH}

cmake --version

cmake \
  "$@" \
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

mkdir -p $OSPRAY_BUILD_DIR
cd $OSPRAY_BUILD_DIR

# Clean out build directory to be sure we are doing a fresh build
rm -rf *

# Setup environment variables for dependencies
export OSPCOMMON_TBB_ROOT=$DEP_DIR
export ospcommon_DIR=$DEP_DIR
export embree_DIR=$DEP_DIR
export glfw3_DIR=$DEP_DIR
export openvkl_DIR=$DEP_DIR
export OpenImageDenoise_DIR=$DEP_DIR

# set release settings
cmake -L \
  -D CMAKE_INSTALL_PREFIX=$INSTALL_DIR \
  -D ISPC_EXECUTABLE=$DEP_DIR/bin/ispc \
  -D OSPRAY_BUILD_ISA=ALL \
  -D OSPRAY_MODULE_DENOISER=ON \
  -D OSPRAY_INSTALL_DEPENDENCIES=OFF \
  -D OSPRAY_ZIP_MODE=ON \
  -D CPACK_PACKAGING_INSTALL_PREFIX=/ \
  -D CMAKE_INSTALL_INCLUDEDIR=include \
  -D CMAKE_INSTALL_LIBDIR=lib \
  -D CMAKE_INSTALL_DOCDIR=doc \
  -D CMAKE_INSTALL_BINDIR=bin \
  ..

# build OSPRay
make -j $THREADS install

# verify libs
check_symbols $INSTALL_DIR/lib/libospray.so GLIBC   2 17 0
check_symbols $INSTALL_DIR/lib/libospray.so GLIBCXX 3 4 19
check_symbols $INSTALL_DIR/lib/libospray.so CXXABI  1 3 7

check_symbols $INSTALL_DIR/lib/libospray_module_ispc.so GLIBC   2 17 0
check_symbols $INSTALL_DIR/lib/libospray_module_ispc.so GLIBCXX 3 4 19
check_symbols $INSTALL_DIR/lib/libospray_module_ispc.so CXXABI  1 3 7

check_imf $INSTALL_DIR/lib/libospray.so
check_imf $INSTALL_DIR/lib/libospray_module_ispc.so

# copy dependent libs into the install
INSTALL_LIB_DIR=$INSTALL_DIR/lib

cp -P $DEP_DIR/lib/*ospcommon.so* $INSTALL_LIB_DIR
cp -P $DEP_DIR/lib/*openvkl*.so* $INSTALL_LIB_DIR
cp -P $DEP_BUILD_DIR/embree/src/lib/*embree*.so* $INSTALL_LIB_DIR
cp -P $DEP_BUILD_DIR/oidn/src/lib/*OpenImage*.so* $INSTALL_LIB_DIR
cp -P $DEP_BUILD_DIR/tbb/src/tbb/lib/intel64/gcc4.8/libtbb.so.* $INSTALL_LIB_DIR
cp -P $DEP_BUILD_DIR/tbb/src/tbb/lib/intel64/gcc4.8/libtbbmalloc.so.* $INSTALL_LIB_DIR

# tar up the results
cd $INSTALL_DIR/..
tar -caf $OSPRAY_PKG_BASE.tar.gz $OSPRAY_PKG_BASE
mv *.tar.gz $ROOT_DIR
