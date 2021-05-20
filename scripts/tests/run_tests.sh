#!/bin/bash
## Copyright 2016-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# to run:  ./run_tests.sh <path to ospray source> <reference images ISA> [TEST_MPI]
# a new folder is created called build_regression_tests with results

if  [ -z "$1" ]; then
  echo "usage: run_tests.sh <OSPRAY_SOURCE_DIR> <OSPRAY_TEST_ISA> [TEST_MPI]"
  exit -1
fi

# Expand relative paths.
SOURCEDIR=$([[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}")

mkdir build_regression_tests
cd build_regression_tests
mkdir failed

cmake -D OSPRAY_TEST_ISA=$2 ${SOURCEDIR}/test_image_data

make -j 4 ospray_test_data

ospTestSuite --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed || exit 2

if [ "$3" == "TEST_MPI" ]; then
    mpirun -n 2 ospTestSuite --osp:load-modules=mpi --osp:device=mpiOffload --gtest_output=xml:tests-mpi.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-mpi
fi

exit $?
