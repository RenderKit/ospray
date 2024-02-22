## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# to run:  ./run_tests.ps1 <path to ospray source> <reference images ISA> [SKIP_CPU] [TEST_MPI] [TEST_MULTIDEVICE]
# a new folder is created called build_regression_tests with results

$osprayDir=$args[0]
$testISA=$args[1]

$testCPU = $TRUE
$testMPI = $FALSE
$testMultiDevice = $FALSE
foreach ($arg in $args) {
  if ($arg -eq "SKIP_CPU") {
    $testCPU = $FALSE
  }
  if ($arg -eq "TEST_MPI") {
    $testMPI = $TRUE
  }
  if ($arg -eq "TEST_MULTIDEVICE") {
    $testMultiDevice = $TRUE
  }
}

md build_regression_tests
cd build_regression_tests

$exitCode = 0

# try Ninja first
cmake -G Ninja -D OSPRAY_TEST_ISA=$testISA $osprayDir/test_image_data
if ($LastExitCode) {
  cmake -D OSPRAY_TEST_ISA=$testISA $osprayDir/test_image_data
  if ($LastExitCode) { $exitCode++ }
}
cmake --build . --config Release --target ospray_test_data
if ($LastExitCode) { $exitCode++ }

if ($testCPU) {
  md failed
  ospTestSuite.exe --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline\ --failed-dir=failed
  if ($LastExitCode) { $exitCode++ }
}

if ($testMultiDevice) {
  md failed-multidevice
  $env:OSPRAY_NUM_SUBDEVICES=2
  ospTestSuite.exe --osp:load-modules=multidevice_cpu --osp:device=multidevice --gtest_output=xml:tests-multidevice.xml --baseline-dir=regression_test_baseline\ --failed-dir=failed-multidevice --gtest_filter="-$test_filters"
  if ($LastExitCode) { $exitCode++ }
}

if ($testMPI) {
  md failed-mpi
  $test_filters += ":TestScenesVariance/*"
  mpiexec.exe -np 1 ospTestSuite.exe --osp:load-modules=mpi_offload --osp:device=mpiOffload --gtest_output=xml:tests-mpi.xml --baseline-dir=regression_test_baseline\ --failed-dir=failed-mpi --gtest_filter="-$test_filters" : -np 2 ospray_mpi_worker.exe
  if ($LastExitCode) { $exitCode++ }

  md failed-mpi-data-parallel
  mpiexec.exe -np 3 -prepend-rank ospMPIDistribTestSuite.exe --gtest_output=xml:tests-mpi.xml --baseline-dir=regression_test_baseline\ --failed-dir=failed-mpi-data-parallel
  if ($LastExitCode) { $exitCode++ }
}

exit $exitCode
