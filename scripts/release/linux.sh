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

#### Build dependencies ####

mkdir deps_build
cd deps_build

# NOTE(jda) - Some Linux OSs need to have lib/ on LD_LIBRARY_PATH at build time
export LD_LIBRARY_PATH=$DEP_DIR/lib:${LD_LIBRARY_PATH}

cmake --version

cmake \
  -DBUILD_DEPENDENCIES_ONLY=ON \
  -DCMAKE_INSTALL_PREFIX=$DEP_DIR \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DINSTALL_IN_SEPARATE_DIRECTORIES=OFF \
  -DDOWNLOAD_ISPC=OFF \
  "$@" ../scripts/superbuild

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

# set release and RPM settings
cmake -L \
-D OSPRAY_BUILD_ISA=ALL \
-D OSPRAY_ZIP_MODE=OFF \
-D OSPRAY_INSTALL_DEPENDENCIES=OFF \
-D CPACK_PACKAGING_INSTALL_PREFIX=/usr \
..

# create RPM files
make -j `nproc` preinstall

check_symbols libospray.so GLIBC   2 4 0
check_symbols libospray.so GLIBCXX 3 4 11
check_symbols libospray.so CXXABI  1 3 0

make -j `nproc` package || exit 2

# change settings for zip mode
cmake -L \
-D OSPRAY_ZIP_MODE=ON \
-D OSPRAY_INSTALL_DEPENDENCIES=ON \
-D CPACK_PACKAGING_INSTALL_PREFIX=/ \
-D CMAKE_INSTALL_INCLUDEDIR=include \
-D CMAKE_INSTALL_LIBDIR=lib \
-D CMAKE_INSTALL_DOCDIR=doc \
-D CMAKE_INSTALL_BINDIR=bin \
..

# create tar.gz files
make -j `nproc` package || exit 2

