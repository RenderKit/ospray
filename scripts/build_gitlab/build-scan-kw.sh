#!/bin/bash

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

cmake -L \
  -DBUILD_DEPENDENCIES_ONLY=ON \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DINSTALL_IN_SEPARATE_DIRECTORIES=OFF \
  "$@" ../scripts/superbuild

cmake --build .

mkdir ospray_build
cd ospray_build

export OSPCOMMON_TBB_ROOT=`pwd`/../install
export ospcommon_DIR=`pwd`/../install
export glfw3_DIR=`pwd`/../install
export embree_DIR=`pwd`/../install
export openvkl_DIR=`pwd`/../install

cmake -DISPC_EXECUTABLE=`pwd`/../install/bin/ispc ../..

$KW_CLIENT_PATH/bin/kwinject make -j `nproc`
$KW_SERVER_PATH/bin/kwbuildproject --url http://$KW_SERVER_IP:$KW_SERVER_PORT/$KW_PROJECT_NAME --tables-directory $CI_PROJECT_DIR/kw_tables kwinject.out
$KW_SERVER_PATH/bin/kwadmin --url http://$KW_SERVER_IP:$KW_SERVER_PORT/ load --force --name build-$CI_JOB_ID $KW_PROJECT_NAME $CI_PROJECT_DIR/kw_tables
echo "build-$CI_JOB_ID" > $CI_PROJECT_DIR/kw_build_number

