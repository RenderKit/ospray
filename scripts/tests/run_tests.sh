#!/bin/bash
## Copyright 2016-2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

export LD_LIBRARY_PATH=`pwd`/build/install/lib:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=`pwd`/build/install/lib:$DYLD_LIBRARY_PATH
export PATH=`pwd`/build/install/bin:$PATH

mkdir build_regression_tests
cd build_regression_tests

cmake ../test_image_data

make -j 4 ospray_test_data

rm -rf failed
mkdir failed

ospTestSuite --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed --gtest_filter=-TestScenesVolumes/FromOsprayTesting.test_scenes/2

exit $?
