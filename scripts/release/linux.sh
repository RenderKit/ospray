#!/bin/bash
## Copyright 2014 Intel Corporation
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

function check_lib_dependency_error
{
for lib in "$1"
do
  if [ -n "`ldd $lib | fgrep $2`" ]; then
    echo "Error: dependency to '$2' found"
    exit 3
  fi
done
}

function get_so_version
{
  local version=`ldd $1 | fgrep $2 | sed "s/.*\.so\.\([0-9\.]\+\).*/\1/"`
  echo "$version"
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
  -D BUILD_OIDN=ON \
  -D BUILD_OSPRAY_MODULE_MPI=ON \
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

# set release settings
cmake -L \
  -D OSPRAY_BUILD_ISA=ALL \
  -D TBB_ROOT=$DEP_DIR \
  -D OSPRAY_ZIP_MODE=ON \
  -D OSPRAY_MODULE_DENOISER=ON \
  -D OSPRAY_INSTALL_DEPENDENCIES=ON \
  -D OSPRAY_MODULE_MPI=ON \
  -D CMAKE_INSTALL_INCLUDEDIR=include \
  -D CMAKE_INSTALL_LIBDIR=lib \
  -D CMAKE_INSTALL_DOCDIR=doc \
  -D CMAKE_INSTALL_BINDIR=bin \
  -D CMAKE_BUILD_WITH_INSTALL_RPATH=ON \
  ..

# create tar.gz
make -j $THREADS preinstall

# verify libs
for lib in libospray.so libospray_module_cpu.so libospray_module_mpi_offload.so libospray_module_mpi_distributed_cpu.so ; do
  echo "checking $lib..."
  check_symbols $lib GLIBC   2 28 0
  check_symbols $lib GLIBCXX 3 4 22
  check_symbols $lib CXXABI  1 3 11
  check_lib_dependency_error $lib libimf.so
  check_lib_dependency_error $lib libsvml.so
done

for lib in libospray_module_mpi_offload.so libospray_module_mpi_distributed_cpu.so ; do
  echo "checking $lib..."
  check_lib_dependency_error $lib libmpifort.so
  
  MPICH_ABI_VER=12
  mpi_so_ver=$(get_so_version $lib libmpi.so)
  mpicxx_so_ver=$(get_so_version $lib libmpicxx.so)
  
  if [ -z "$mpi_so_ver" ] || [ -z "$mpicxx_so_ver" ]; then
    echo "MPI module is not linked against libmpi.so or libmpicxx.so"
    exit 3
  fi
  
  if [ "$mpi_so_ver" != "$MPICH_ABI_VER" ]; then
    echo "MPI module is linked against the wrong MPICH ABI: libmpi.so.$mpi_so_ver"
    exit 3
  fi
  if [ "$mpicxx_so_ver" != "$MPICH_ABI_VER" ]; then
    echo "MPI module is linked against the wrong MPICH ABI: libmpicxx.so.$mpi_so_ver"
    exit 3
  fi
done

make -j $THREADS package || exit 2
