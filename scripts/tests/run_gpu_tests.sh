#!/bin/bash
## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# to run:  ./run_tests.sh <path to ospray source> [SKIP_GPU] [TEST_MPI] [TEST_MULTIDEVICE]
# a new folder is created called build_regression_tests with results

SOURCEDIR=$([[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}")

if [ -z "$MPI_ROOT_CONFIG" ]; then
  MPI_ROOT_CONFIG="-np 1"
fi
if [ -z "$MPI_WORKER_CONFIG" ]; then
  MPI_WORKER_CONFIG="-np 2"
fi

# optional command line arguments
TEST_GPU=true
while [[ $# -gt 0 ]]
do
key="$1"
case $key in
    SKIP_GPU)
    unset TEST_GPU
    shift
    ;;
    TEST_MPI)
    TEST_MPI=true
    shift
    ;;
    TEST_MULTIDEVICE)
    TEST_MULTIDEVICE=true
    shift
    ;;
    *)
    shift
    ;;
esac
done

mkdir build_regression_tests
cd build_regression_tests

exitCode=0

cmake -D OSPRAY_TEST_ISA=AVX512SKX "${SOURCEDIR}/test_image_data"
let exitCode+=$?
export CMAKE_BUILD_PARALLEL_LEVEL=32
cmake --build . --target ospray_test_data
let exitCode+=$?

### Excluded tests on GPU
#########################
# Clipping unsupported
test_filters="ClippingParallel.planes"
test_filters+=":TestScenesClipping/FromOsprayTesting.*"
test_filters+=":TestScenesMaxDepth/FromOsprayTestingMaxDepth.test_scenes/1"
test_filters+=":TestScenesMaxDepth/FromOsprayTestingMaxDepth.test_scenes/2"
test_filters+=":TestMotionBlur/MotionBlurBoxes.*"
test_filters+=":CameraRollingShutter/MotionBlurBoxes.*"
test_filters+=":CameraStereoRollingShutter/MotionBlurBoxes.*"
# Subdivision surfaces unsupported
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/15"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/16"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/17"
test_filters+=":Color/Interpolation.Interpolation/4"
test_filters+=":Color/Interpolation.Interpolation/5"
test_filters+=":Color/Interpolation.Interpolation/6"
test_filters+=":Color/Interpolation.Interpolation/7"
test_filters+=":Texcoord/Interpolation.Interpolation/2"
test_filters+=":Texcoord/Interpolation.Interpolation/3"
# Multiple volumes unsupported
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/3"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/4"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/5"
test_filters+=":TestScenesVolumesStrictParams/FromOsprayTesting.*"
# Requires non-overlapping multiple volume support on GPU
test_filters+=":ObjectInstance/IDBuffer.*"
# Instancing test includes multiple volumes
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/24"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/25"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/26"
# 'mix' material not supported on GPU (not practical to implement without fn ptr)
test_filters+=":TestScenesPtMaterials/FromOsprayTesting.test_scenes/8"
# Crashing FIXME
test_filters+=":Primitive/IDBuffer.*"
# Different noise
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/1"

export ONEAPI_DEVICE_SELECTOR=level_zero:*
export SYCL_CACHE_PERSISTENT=1
export OIDN_VERBOSE=2

export ZE_FLAT_DEVICE_HIERARCHY=COMPOSITE # WA for PVC

if [ $TEST_GPU ]; then
  mkdir failed-gpu
  ospTestSuite --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-gpu --osp:load-modules=gpu --osp:device=gpu --gtest_filter="-$test_filters" --own-SYCL
  let exitCode+=$?
  
  OSPRAY_ALLOW_DEVICE_MEMORY=1 ospTestSuite --baseline-dir=regression_test_baseline/ --failed-dir=failed-gpu --osp:load-modules=gpu --osp:device=gpu --gtest_filter=SharedData/TestUSMSharing.structured_regular/2 --own-SYCL
  let exitCode+=$?
fi

if [ $TEST_MULTIDEVICE ]; then
  mkdir failed-multidevice
  OSPRAY_NUM_SUBDEVICES=2 ospTestSuite --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-multidevice --gtest_filter="-$test_filters" --osp:load-modules=multidevice_gpu --osp:device=multidevice --own-SYCL
  let exitCode+=$?

  OSPRAY_ALLOW_DEVICE_MEMORY=1 OSPRAY_NUM_SUBDEVICES=2 ospTestSuite --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-multidevice --gtest_filter=SharedData/TestUSMSharing.structured_regular/2 --osp:load-modules=multidevice_gpu --osp:device=multidevice --own-SYCL
  let exitCode+=$?
fi

if [ $TEST_MPI ]; then
  mkdir failed-mpi-gpu
  # Need to export, not just set for MPI to pick it up
  export OSPRAY_MPI_DISTRIBUTED_GPU=1
  mpiexec $MPI_ROOT_CONFIG ospTestSuite --gtest_output=xml:tests-mpi-offload.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-mpi-gpu --osp:load-modules=mpi_offload --osp:device=mpiOffload --gtest_filter="-$test_filters" : $MPI_WORKER_CONFIG ospray_mpi_worker
  let exitCode+=$?

  mkdir failed-mpi-gpu-data-parallel
  test_filters="MPIDistribTestScenesVolumes/MPIFromOsprayTesting.test_scenes/1" # FIXME
  mpiexec -np 3 ospMPIDistribTestSuite --gtest_output=xml:tests-mpi-distrib.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-mpi-gpu-data-parallel --gtest_filter="-$test_filters"
  let exitCode+=$?
fi

exit $exitCode
