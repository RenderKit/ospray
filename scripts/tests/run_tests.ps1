## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# to run:  ./run_tests.ps1 <path to ospray source> <reference images ISA>
# a new folder is created called build_regression_tests with results

$osprayDir=$args[0]
$testISA=$args[1]

md build_regression_tests
cd build_regression_tests
md failed

cmake -D OSPRAY_TEST_ISA=$testISA $osprayDir/test_image_data

cmake --build . --config Release --target ospray_test_data

if ( $env:MPI -eq "ON" ) {
    mpiexec.exe -n 2 ospTestSuite.exe --osp:load-modules=mpi --osp:device=mpiOffload --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline\ --failed-dir=failed\
} else {
    ospTestSuite.exe --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline\ --failed-dir=failed\
}

exit $LastExitCode
