#!/bin/bash
## Copyright 2019-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set -e

KW_SERVER_PATH=$KW_PATH/server
KW_CLIENT_PATH=$KW_PATH/client
export KLOCWORK_LTOKEN=/tmp/ltoken

echo "$KW_SERVER_IP;$KW_SERVER_PORT;$KW_USER;$KW_LTOKEN" > $KLOCWORK_LTOKEN


mkdir build
cd build

# NOTE(jda) - Some Linux OSs need to have lib/ on LD_LIBRARY_PATH at build time
export LD_LIBRARY_PATH=`pwd`/install/lib:${LD_LIBRARY_PATH}

cmake --version

# NOTE: temporarily using older VKL version while waiting to move to new iterator API
cmake -L \
  -DBUILD_DEPENDENCIES_ONLY=ON \
  -DBUILD_EMBREE_FROM_SOURCE=OFF \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DBUILD_OPENVKL_VERSION=679d598d \
  "$@" ../scripts/superbuild

cmake --build .

INSTALL_DIR=`pwd`/install

mkdir module_build
cd module_build

export rkcommon_DIR=$INSTALL_DIR
export glfw3_DIR=$INSTALL_DIR
export embree_DIR=$INSTALL_DIR
export openvkl_DIR=$INSTALL_DIR
export ospray_DIR=$INSTALL_DIR

cmake -DISPC_EXECUTABLE=$INSTALL_DIR/bin/ispc -DTBB_ROOT=$INSTALL_DIR ../..

$KW_CLIENT_PATH/bin/kwinject make -j `nproc`
$KW_SERVER_PATH/bin/kwbuildproject --url http://$KW_SERVER_IP:$KW_SERVER_PORT/$KW_PROJECT_NAME --tables-directory $CI_PROJECT_DIR/kw_tables kwinject.out
$KW_SERVER_PATH/bin/kwadmin --url http://$KW_SERVER_IP:$KW_SERVER_PORT/ load --force --name build-$CI_JOB_ID $KW_PROJECT_NAME $CI_PROJECT_DIR/kw_tables
echo "build-$CI_JOB_ID" > $CI_PROJECT_DIR/kw_build_number

