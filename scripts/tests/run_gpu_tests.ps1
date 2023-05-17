## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# to run:  ./run_tests.ps1 <path to ospray source> <reference images ISA> [TEST_MPI]
# a new folder is created called build_regression_tests with results

$osprayDir=$args[0]

$testMPI = $FALSE
$testMultiDevice = $FALSE
foreach ($arg in $args) {
  if ( $arg -eq "TEST_MPI" ) {
    $testMPI = $TRUE
  }
  if ( $arg -eq "TEST_MULTIDEVICE" ) {
    $testMultiDevice = $TRUE
  }
}

md build_regression_tests
cd build_regression_tests

$exitCode = 0

cmake -D OSPRAY_TEST_ISA=AVX512SKX $osprayDir/test_image_data
$exitCode += $LastExitCode
cmake --build . --config Release --target ospray_test_data
$exitCode += $LastExitCode

# Excluded tests on GPU
$test_filters="ClippingParallel.planes"
# Subdivision surfaces unsupported
$test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/15"
$test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/16"
$test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/17"
# Artifacts on PVC
$test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/21"
$test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/22"
# Instancing test also makes use of motion blur, which is unsupported
$test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/24"
$test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/25"
$test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/26"
# Clipping unsupported
$test_filters+=":TestScenesClipping/FromOsprayTesting.*"
# Different noise
$test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/1"
# Multiple volumes, unsupported
$test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/3"
$test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/4"
$test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/5"
# Line artifacts
$test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/18"
# Multiple volumes unsupported
$test_filters+=":TestScenesVolumesStrictParams/FromOsprayTesting.*"
# Clipping unsupported
$test_filters+=":TestScenesMaxDepth/FromOsprayTestingMaxDepth.test_scenes/1"
$test_filters+=":TestScenesMaxDepth/FromOsprayTestingMaxDepth.test_scenes/2"
# Almost all working, some remaining bug on cell-valued unstructured volumes
$test_filters+=":TestScenesVolumes/UnstructuredVolume.1"
$test_filters+=":TestScenesVolumes/UnstructuredVolume.3"
# Motion blur unsupported
$test_filters+=":TestMotionBlur/MotionBlurBoxes.*"
$test_filters+=":CameraRollingShutter/MotionBlurBoxes.*"
$test_filters+=":CameraStereoRollingShutter/MotionBlurBoxes.*"
$test_filters+=":Camera/MotionCamera.*"
$test_filters+=":CameraOrtho/MotionCamera.*"
$test_filters+=":CameraStereoRollingShutter/MotionCamera.*"
$test_filters+=":LightMotionBlur/*"
# Crashing
$test_filters+=":Primitive/IDBuffer.*"
# Requires non-overlapping multiple volume support on GPU
$test_filters+=":ObjectInstance/IDBuffer.*"
# Subdivision surfaces not supported on GPU
$test_filters+=":Color/Interpolation.Interpolation/4"
$test_filters+=":Color/Interpolation.Interpolation/5"
$test_filters+=":Color/Interpolation.Interpolation/6"
$test_filters+=":Color/Interpolation.Interpolation/7"
$test_filters+=":Texcoord/Interpolation.Interpolation/2"
$test_filters+=":Texcoord/Interpolation.Interpolation/3"
# Variance termination is not quite right
$test_filters+=":TestScenesVariance/FromOsprayTestingVariance.testScenes/0"
# 'mix' material not supported on GPU (difficult to implement without fn ptr)
$test_filters+=":TestScenesPtMaterials/FromOsprayTesting.test_scenes/13"

$env:ONEAPI_DEVICE_SELECTOR="level_zero:*"

$env:OIDN_VERBOSE="2"

md failed-gpu
ospTestSuite.exe --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline\ --failed-dir=failed-gpu --osp:load-modules=gpu --osp:device=gpu --gtest_filter="-$test_filters" --own-SYCL
$exitCode += $LastExitCode

$env:OSPRAY_ALLOW_DEVICE_MEMORY = "1"
ospTestSuite.exe --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline\ --failed-dir=failed-gpu --osp:load-modules=gpu --osp:device=gpu --gtest_filter=SharedData/TestUSMSharing.structured_regular/2 --own-SYCL
$exitCode += $LastExitCode
$env:OSPRAY_ALLOW_DEVICE_MEMORY = "0"

if ( $testMultiDevice ) {
  md failed-multidevice
  $Env:OSPRAY_NUM_SUBDEVICES = 2
  $test_filters = "DebugOp/ImageOp.ImageOp/0" # post-processing not enabled on multidevice
  ospTestSuite.exe --osp:load-modules=multidevice_cpu --osp:device=multidevice --gtest_output=xml:tests-multidevice.xml --baseline-dir=regression_test_baseline\ --failed-dir=failed-multidevice --gtest_filter="-$test_filters"
  $exitCode += $LastExitCode
}

if ( $testMPI ) {
  md failed-mpi
  $test_filters += ":TestScenesVariance/*"
  mpiexec.exe -n 2 ospTestSuite.exe --osp:load-modules=mpi_offload --osp:device=mpiOffload --gtest_output=xml:tests-mpi.xml --baseline-dir=regression_test_baseline\ --failed-dir=failed-mpi --gtest_filter="-$test_filters"
  $exitCode += $LastExitCode

  md failed-mpi-data-parallel
  mpiexec.exe -n 2 ospMPIDistribTestSuite.exe --gtest_output=xml:tests-mpi.xml --baseline-dir=regression_test_baseline\ --failed-dir=failed-mpi-data-parallel
  $exitCode += $LastExitCode
}

exit $exitCode
