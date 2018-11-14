#!/bin/bash
## ======================================================================== ##
## Copyright 2016-2018 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

rm -rf failed
mkdir failed

cd build
cmake \
-D OSPRAY_ENABLE_TESTING=ON \
..

make -j 4 ospray_test_data
cd ..

build/regression_tests/ospray_test_suite --gtest_output=xml:build/tests.xml --baseline-dir=build/regression_tests/baseline/ --failed-dir=build/failed
FAILED=$(echo $?)

exit $FAILED

