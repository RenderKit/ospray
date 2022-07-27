#!/bin/bash
## Copyright 2016 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# to run:  ./run_tests.sh <path to ospray source> <reference images ISA> [TEST_MPI] [TEST_MULTIDEVICE]
# a new folder is created called build_regression_tests with results

if [ -z "$MPI_ROOT_CONFIG" ]; then
  MPI_ROOT_CONFIG="-np 1"
fi
if [ -z "$MPI_WORKER_CONFIG" ]; then
  MPI_WORKER_CONFIG="-np 1"
fi

if  [ -z "$1" ]; then
  echo "usage: run_tests.sh <OSPRAY_SOURCE_DIR> <OSPRAY_TEST_ISA> [TEST_MPI] [TEST_MULTIDEVICE]"
  exit -1
fi

# Expand relative paths.
SOURCEDIR=$([[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}")

mkdir build_regression_tests
cd build_regression_tests

cmake -D OSPRAY_TEST_ISA=$2 ${SOURCEDIR}/test_image_data

# optional command line arguments
while [[ $# -gt 0 ]]
do
key="$1"
case $key in
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

make -j 4 ospray_test_data

mkdir failed
ospTestSuite --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed || exit 2

if [ $TEST_MULTIDEVICE ]; then
  mkdir failed-multidevice
  OSPRAY_NUM_SUBDEVICES=2 ospTestSuite --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-multidevice --osp:load-modules=multidevice --osp:device=multidevice || exit 2
fi

if [ $TEST_MPI ]; then
  mkdir failed-mpi
  mpiexec $MPI_ROOT_CONFIG ospTestSuite --gtest_output=xml:tests-mpi.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-mpi --osp:load-modules=mpi --osp:device=mpiOffload : $MPI_WORKER_CONFIG ospray_mpi_worker || exit 2

  mkdir failed-mpi-data-parallel
  mpiexec $MPI_ROOT_CONFIG ospMPIDistribTestSuite --gtest_output=xml:tests-mpi.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-mpi-data-parallel || exit 2
fi

exit $?
