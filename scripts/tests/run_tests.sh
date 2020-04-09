#!/bin/bash
## Copyright 2016-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# to run:  ./run_tests.sh <path to ospray source>
# a new folder is created called build_regression_tests with results

mkdir build_regression_tests
cd build_regression_tests
mkdir failed

cmake $1/test_image_data

make -j 4 ospray_test_data

ospTestSuite --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed

exit $?
