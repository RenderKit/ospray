#!/bin/bash
## Copyright 2016 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# to run:  ./run_tests.sh <path to ospray source> <reference images ISA> [SKIP_CPU] [TEST_MPI] [TEST_MULTIDEVICE]
# a new folder is created called build_regression_tests with results

if [ -z "$MPI_ROOT_CONFIG" ]; then
  MPI_ROOT_CONFIG="-np 1"
fi
if [ -z "$MPI_WORKER_CONFIG" ]; then
  MPI_WORKER_CONFIG="-np 2"
fi

if  [ -z "$1" ]; then
  echo "usage: run_tests.sh <OSPRAY_SOURCE_DIR> <OSPRAY_TEST_ISA> [TEST_MPI] [TEST_MULTIDEVICE]"
  exit -1
fi

# Expand relative paths.
SOURCEDIR=$([[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}")
TESTISA=$2

# optional command line arguments
TEST_CPU=true
while [[ $# -gt 0 ]]
do
key="$1"
case $key in
    SKIP_CPU)
    unset TEST_CPU
    shift
    ;;
    TEST_MPI)
    TEST_MPI=true
    shift
    ;;
    TEST_MULTIDEVICE)
    TEST_MULTIDEVICE=true
    shift
    ;;
    *)
    shift
    ;;
esac
done

mkdir build_regression_tests
cd build_regression_tests

exitCode=0

cmake -D OSPRAY_TEST_ISA=$TESTISA ${SOURCEDIR}/test_image_data
let exitCode+=$?
export CMAKE_BUILD_PARALLEL_LEVEL=32
cmake --build . --target ospray_test_data
let exitCode+=$?

export OIDN_VERBOSE=2

if [ $TEST_CPU ]; then
  mkdir failed
  ospTestSuite --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed
  let exitCode+=$?
fi

if [ $TEST_MULTIDEVICE ]; then
  mkdir failed-multidevice
  OSPRAY_NUM_SUBDEVICES=2 ospTestSuite --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-multidevice --gtest_filter="-$test_filters" --osp:load-modules=multidevice_cpu --osp:device=multidevice
  let exitCode+=$?
fi

if [ $TEST_MPI ]; then
  mkdir failed-mpi
  test_filters="TestScenesVariance/*"
  mpiexec $MPI_ROOT_CONFIG ospTestSuite --gtest_output=xml:tests-mpi-offload.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-mpi --gtest_filter="-$test_filters" --osp:load-modules=mpi_offload --osp:device=mpiOffload : $MPI_WORKER_CONFIG ospray_mpi_worker
  let exitCode+=$?

  mkdir failed-mpi-data-parallel
  mpiexec -np 3 ospMPIDistribTestSuite --gtest_output=xml:tests-mpi-distrib.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-mpi-data-parallel
  let exitCode+=$?
fi

exit $exitCode
