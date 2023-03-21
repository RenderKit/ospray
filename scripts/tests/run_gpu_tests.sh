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

cmake -D OSPRAY_TEST_ISA=AVX2 "${SOURCEDIR}/test_image_data"
make -j 4 ospray_test_data

# Excluded tests on GPU
test_filters="ClippingParallel.planes"
test_filters+=":Intersection/SpherePrecision.sphere/8"
test_filters+=":Intersection/SpherePrecision.sphere/9"
test_filters+=":Intersection/SpherePrecision.sphere/10"
test_filters+=":Intersection/SpherePrecision.sphere/11"
test_filters+=":Intersection/SpherePrecision.sphere/20"
test_filters+=":Intersection/SpherePrecision.sphere/21"
test_filters+=":Intersection/SpherePrecision.sphere/22"
test_filters+=":Intersection/SpherePrecision.sphere/23"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/3"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/4"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/5"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/15"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/16"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/17"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/21"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/22"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/23"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/24"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/25"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/26"
test_filters+=":TestScenesClipping/FromOsprayTesting.*"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/1"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/3"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/4"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/5"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/6"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/7"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/8"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/9"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/10"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/11"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/12"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/13"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/14"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/15"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/16"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/17"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/18"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/19"
test_filters+=":TestScenesVolumes/FromOsprayTesting.test_scenes/20"
test_filters+=":TestScenesVolumesStrictParams/FromOsprayTesting.*"
test_filters+=":Transparency/FromOsprayTesting.*"
test_filters+=":TestScenesMaxDepth/FromOsprayTestingMaxDepth.test_scenes/1"
test_filters+=":TestScenesMaxDepth/FromOsprayTestingMaxDepth.test_scenes/2"
test_filters+=":TestScenesVolumes/UnstructuredVolume.*"
test_filters+=":TestMotionBlur/MotionBlurBoxes.*"
test_filters+=":CameraRollingShutter/MotionBlurBoxes.*"
test_filters+=":CameraStereoRollingShutter/MotionBlurBoxes.*"
test_filters+=":Camera/MotionCamera.*"
test_filters+=":CameraOrtho/MotionCamera.*"
test_filters+=":CameraStereoRollingShutter/MotionCamera.*"
test_filters+=":LightMotionBlur/*"
test_filters+=":Primitive/IDBuffer.*"
test_filters+=":ObjectInstance/IDBuffer.*"
test_filters+=":Color/Interpolation.Interpolation/4"
test_filters+=":Color/Interpolation.Interpolation/5"
test_filters+=":Color/Interpolation.Interpolation/6"
test_filters+=":Color/Interpolation.Interpolation/7"
test_filters+=":Texcoord/Interpolation.Interpolation/2"
test_filters+=":Texcoord/Interpolation.Interpolation/3"

export ONEAPI_DEVICE_SELECTOR=level_zero:*

mkdir failed-gpu

ospTestSuite --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-gpu --osp:load-modules=gpu --osp:device=gpu --gtest_filter="-$test_filters"

if [ $TEST_MPI ]; then
  mkdir failed-mpi-gpu
  # Need to export, not just set for MPI to pick it up
  export OSPRAY_MPI_DISTRIBUTED_GPU=1
  mpiexec $MPI_ROOT_CONFIG ospTestSuite --gtest_output=xml:tests-mpi-offload.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-mpi-gpu --osp:load-modules=mpi_offload --osp:device=mpiOffload --gtest_filter="-$test_filters" : $MPI_WORKER_CONFIG ospray_mpi_worker

  mkdir failed-mpi-gpu-data-parallel
  mpiexec $MPI_ROOT_CONFIG ospMPIDistribTestSuite --gtest_output=xml:tests-mpi-distrib.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-mpi-gpu-data-parallel --gtest_filter="MPIDistribTestScenesGeometry*:MPIDistribTestScenesVolumes*test_scenes/0"
fi

