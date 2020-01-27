## Copyright 2009-2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

echo Running tests

$PATH += ";..\build\install\bin"

md build_regression_tests
cd build_regression_tests

md failed

cmake ../test_image_data

cmake --build . --config Release --target ospray_test_data

..\build\install\bin\ospTestSuite.exe --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline\ --failed-dir=failed\ --gtest_filter=-TestScenesVolumes/FromOsprayTesting.test_scenes/2

exit $LastExitCode
