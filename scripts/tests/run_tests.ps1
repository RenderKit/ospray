## ======================================================================== ##
## Copyright 2009-2019 Intel Corporation                                    ##
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

echo Running tests

$PATH += ";..\build\install\bin"

md build_regression_tests
cd build_regression_tests

md failed

cmake ../test_image_data

cmake --build . --config Release --target ospray_test_data

..\build\install\bin\ospTestSuite.exe --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline\ --failed-dir=failed\

exit $LastExitCode
