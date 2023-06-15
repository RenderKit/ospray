#!/bin/bash
## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# to run:  ./run_tests.sh <path to ospray source> [TEST_MPI] [TEST_MULTIDEVICE]

SOURCEDIR=$([[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}")

if [ -z "$MPI_ROOT_CONFIG" ]; then
  MPI_ROOT_CONFIG="-np 1"
fi
if [ -z "$MPI_WORKER_CONFIG" ]; then
  MPI_WORKER_CONFIG="-np 1"
fi

# optional command line arguments
while [[ $# -gt 0 ]]
do
key="$1"
case $key in
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

cmake -D OSPRAY_TEST_ISA=AVX512SKX "${SOURCEDIR}/test_image_data"
make -j 4 ospray_test_data

# Excluded tests on GPU
test_filters="ClippingParallel.planes"
# Subdivision surfaces unsupported
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/15"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/16"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/17"
# Artifacts on PVC
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/21"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/22"
# Instancing test also makes use of motion blur, which is unsupported
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/24"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/25"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/26"
# Clipping unsupported
test_filters+=":TestScenesClipping/FromOsprayTesting.*"
# Different noise
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/1"
# Multiple volumes, unsupported
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/3"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/4"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/5"
# Line artifacts
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/18"
# Multiple volumes unsupported
test_filters+=":TestScenesVolumesStrictParams/FromOsprayTesting.*"
# Clipping unsupported
test_filters+=":TestScenesMaxDepth/FromOsprayTestingMaxDepth.test_scenes/1"
test_filters+=":TestScenesMaxDepth/FromOsprayTestingMaxDepth.test_scenes/2"
# Almost all working, some remaining bug on cell-valued unstructured volumes
test_filters+=":TestScenesVolumes/UnstructuredVolume.1"
test_filters+=":TestScenesVolumes/UnstructuredVolume.3"
# Motion blur unsupported
test_filters+=":TestMotionBlur/MotionBlurBoxes.*"
test_filters+=":CameraRollingShutter/MotionBlurBoxes.*"
test_filters+=":CameraStereoRollingShutter/MotionBlurBoxes.*"
test_filters+=":Camera/MotionCamera.*"
test_filters+=":CameraOrtho/MotionCamera.*"
test_filters+=":CameraStereoRollingShutter/MotionCamera.*"
test_filters+=":LightMotionBlur/*"
# Crashing
test_filters+=":Primitive/IDBuffer.*"
# Requires non-overlapping multiple volume support on GPU
test_filters+=":ObjectInstance/IDBuffer.*"
# Subdivision surfaces not supported on GPU
test_filters+=":Color/Interpolation.Interpolation/4"
test_filters+=":Color/Interpolation.Interpolation/5"
test_filters+=":Color/Interpolation.Interpolation/6"
test_filters+=":Color/Interpolation.Interpolation/7"
test_filters+=":Texcoord/Interpolation.Interpolation/2"
test_filters+=":Texcoord/Interpolation.Interpolation/3"
# Variance termination is not quite right
test_filters+=":TestScenesVariance/FromOsprayTestingVariance.testScenes/0"

export ONEAPI_DEVICE_SELECTOR=level_zero:*

mkdir failed-gpu

ospTestSuite --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-gpu --osp:load-modules=gpu --osp:device=gpu --gtest_filter="-$test_filters" || exit 2

if [ $TEST_MPI ]; then
  mkdir failed-mpi-gpu
  # Need to export, not just set for MPI to pick it up
  export OSPRAY_MPI_DISTRIBUTED_GPU=1
  mpiexec $MPI_ROOT_CONFIG ospTestSuite --gtest_output=xml:tests-mpi-offload.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-mpi-gpu --osp:load-modules=mpi_offload --osp:device=mpiOffload --gtest_filter="-$test_filters" : $MPI_WORKER_CONFIG ospray_mpi_worker || exit 2

  mkdir failed-mpi-gpu-data-parallel
  mpiexec $MPI_ROOT_CONFIG ospMPIDistribTestSuite --gtest_output=xml:tests-mpi-distrib.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-mpi-gpu-data-parallel --gtest_filter="MPIDistribTestScenesGeometry*:MPIDistribTestScenesVolumes*test_scenes/0" || exit 2 
fi

exit $?
