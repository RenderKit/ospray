#!/bin/bash

# function repeat the command $1 times, when the last line output is equal ERROR_MSG
function retry_cmd()
{
    set +e
    MAX_RETRY=$1
    CMD="${@:2}"
    ERROR_MSG="License check failed ... Exiting"

    RETRY_COUNTER="0"
    while [ $RETRY_COUNTER -lt $MAX_RETRY ]; do
        CMD_OUTPUT=$($CMD)
        CMD_RETURN_CODE=$?
        echo "$CMD_OUTPUT"
        CMD_OUTPUT=$(echo $CMD_OUTPUT | tail -1)
        if [ $CMD_RETURN_CODE == 0 ] && [[ $CMD_OUTPUT != $ERROR_MSG ]]; then
            break
        elif [ $CMD_RETURN_CODE != 1 ]; then
            set -e
            echo "Unknown script error code = [$CMD_RETURN_CODE]"
            return $CMD_RETURN_CODE
        fi
        RETRY_COUNTER=$[$RETRY_COUNTER+1]
        echo "Found license check failed, [$RETRY_COUNTER/$MAX_RETRY] - retrying ... "
        sleep 10
    done

    set -e
    if [ $RETRY_COUNTER -ge $MAX_RETRY ]; then
	return 62
    fi
    return 0
}

set -e

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

# build
$KW_CLIENT_PATH/bin/kwinject make -j `nproc`

retry_cmd 15 $KW_SERVER_PATH/bin/kwbuildproject --url http://$KW_SERVER_IP:$KW_SERVER_PORT/$KW_PROJECT_NAME --tables-directory $CI_PROJECT_DIR/release/kw_tables kwinject.out
retry_cmd 15 $KW_SERVER_PATH/bin/kwadmin --url http://$KW_SERVER_IP:$KW_SERVER_PORT/ load $KW_PROJECT_NAME $CI_PROJECT_DIR/release/kw_tables | tee project_load_log
cat project_load_log | grep "Starting build" | cut -d":" -f2 > $CI_PROJECT_DIR/kw_build_number
