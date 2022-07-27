#!/bin/bash
## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set -e

KW_SERVER_PATH=$KW_PATH/server
KW_CLIENT_PATH=$KW_PATH/client

export KLOCWORK_LTOKEN=/tmp/ltoken
echo "$KW_SERVER_IP;$KW_SERVER_PORT;$KW_USER;$KW_LTOKEN" > $KLOCWORK_LTOKEN

mkdir -p $CI_PROJECT_DIR/klocwork
log_file=$CI_PROJECT_DIR/klocwork/build.log


mkdir build
cd build

# NOTE(jda) - Some Linux OSs need to have lib/ on LD_LIBRARY_PATH at build time
export LD_LIBRARY_PATH=`pwd`/install/lib:${LD_LIBRARY_PATH}

cmake --version

cmake -L \
  -DBUILD_DEPENDENCIES_ONLY=ON \
  -DBUILD_EMBREE_FROM_SOURCE=OFF \
  -DBUILD_OIDN=ON \
  -DBUILD_OIDN_FROM_SOURCE=OFF \
  -DBUILD_OSPRAY_MODULE_MPI=ON \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DINSTALL_IN_SEPARATE_DIRECTORIES=OFF \
  "$@" ../scripts/superbuild

cmake --build .

INSTALL_DIR=`pwd`/install

mkdir ospray_build
cd ospray_build

export CMAKE_PREFIX_PATH=${INSTALL_DIR}

cmake -DISPC_EXECUTABLE=$INSTALL_DIR/bin/ispc -DTBB_ROOT=$INSTALL_DIR -DOSPRAY_MODULE_DENOISER=ON -DOSPRAY_MODULE_BILINEAR_PATCH=ON -DOSPRAY_MODULE_MPI=ON ../..

$KW_CLIENT_PATH/bin/kwinject make -j `nproc` | tee -a $log_file
$KW_SERVER_PATH/bin/kwbuildproject --classic --url http://$KW_SERVER_IP:$KW_SERVER_PORT/$KW_PROJECT_NAME --tables-directory $CI_PROJECT_DIR/kw_tables kwinject.out | tee -a $log_file
$KW_SERVER_PATH/bin/kwadmin --url http://$KW_SERVER_IP:$KW_SERVER_PORT/ load --force --name build-$CI_JOB_ID $KW_PROJECT_NAME $CI_PROJECT_DIR/kw_tables | tee -a $log_file

# Store kw build name for check status later
echo "build-$CI_JOB_ID" > $CI_PROJECT_DIR/klocwork/build_name

