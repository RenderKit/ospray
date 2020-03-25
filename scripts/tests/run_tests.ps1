## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# to run:  ./run_tests.ps1 <path to ospray source>
# a new folder is created called build_regression_tests with results

$osprayDir=$args[0]

md build_regression_tests
cd build_regression_tests
md failed

cmake $osprayDir/test_image_data

cmake --build . --config Release --target ospray_test_data

ospTestSuite.exe --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline\ --failed-dir=failed\

exit $LastExitCode
