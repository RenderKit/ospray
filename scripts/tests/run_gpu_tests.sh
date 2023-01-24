#!/bin/bash
## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

SOURCEDIR=$([[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}")

mkdir build_regression_tests
cd build_regression_tests

cmake -D OSPRAY_TEST_ISA=AVX2 "${SOURCEDIR}/test_image_data"
make -j 4 ospray_test_data

mkdir failed-gpu

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
test_filters+=":TestScenesVolumes/FromOsprayTesting.*"
test_filters+=":TestScenesVolumesStrictParams/FromOsprayTesting.*"
test_filters+=":Transparency/FromOsprayTesting.*"
test_filters+=":TestScenesGeometry/Curves.*"
test_filters+=":TestScenesMaxDepth/FromOsprayTestingMaxDepth.test_scenes/1"
test_filters+=":TestScenesMaxDepth/FromOsprayTestingMaxDepth.test_scenes/2"
test_filters+=":TestScenesVolumes/UnstructuredVolume.*"
test_filters+=":Renderers/TextureVolumeTransform_deprecated.*"
test_filters+=":Renderers/TextureVolumeTransform.*"
test_filters+=":Renderers/DepthCompositeVolume.*"
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

SYCL_DEVICE_FILTER=level_zero
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./ 
ospTestSuite --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-gpu --osp:load-modules=gpu --osp:device=gpu --gtest_filter="-$test_filters"

