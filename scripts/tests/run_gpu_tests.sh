#!/bin/bash
## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

SOURCEDIR=$([[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}")

mkdir build_regression_tests
cd build_regression_tests

cmake -D OSPRAY_TEST_ISA=AVX2 "${SOURCEDIR}/test_image_data"
make -j 4 ospray_test_data

mkdir failed-gpu

test_filters="Enums.*"
test_filters+=":Intersection/SpherePrecision.*"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/0"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/1"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/2"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/6"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/7"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/8"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/9"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/10"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/11"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/12"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/13"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/14"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/18"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/19"
test_filters+=":TestScenesGeometry/FromOsprayTesting.test_scenes/20"
test_filters+=":TestScenesPtMaterials/FromOsprayTesting.*"
test_filters+=":TestScenesGeometry/Curves.*"
test_filters+=":TestScenesMaxDepth/FromOsprayTestingMaxDepth.test_scenes/0"
test_filters+=":MaterialLists/RendererMaterialList.*"
test_filters+=":Appearance/Texture2D.*"
test_filters+=":Appearance/Texture2DTransform.*"
test_filters+=":Appearance/PTBackgroundRefraction.*"
test_filters+=":Light/AmbientLight.*"
test_filters+=":Light/DistantLight.*"
test_filters+=":Light/GeometricLight.*"
test_filters+=":Light/QuadLight.*"
test_filters+=":LightIntensityQuantity/QuadLight.*"
test_filters+=":Light/CylinderLight.*"
test_filters+=":LightIntensityQuantity/CylinderLight.*"
test_filters+=":Light/SphereLight.*"
test_filters+=":LightIntensityQuantity/SphereLight.*"
test_filters+=":Light/SpotLight.*"
test_filters+=":LightIntensityQuantity/SpotLight.*"
test_filters+=":Light/PhotometricLight.*"
test_filters+=":Light/HDRILight.*"
test_filters+=":Light/SunSky.*"
test_filters+=":Light2/SunSky.*"
test_filters+=":Camera/Stereo.*"
test_filters+=":Color/Interpolation.Interpolation/0"
test_filters+=":Color/Interpolation.Interpolation/1"
test_filters+=":Color/Interpolation.Interpolation/2"
test_filters+=":Color/Interpolation.Interpolation/3"
test_filters+=":Texcoord/Interpolation.Interpolation/0"
test_filters+=":Texcoord/Interpolation.Interpolation/1"
test_filters+=":Normal/Interpolation.*"

SYCL_DEVICE_FILTER=level_zero
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./ 
ospTestSuite --gtest_output=xml:tests.xml --baseline-dir=regression_test_baseline/ --failed-dir=failed-gpu --osp:load-modules=gpu --osp:device=gpu --gtest_filter="$test_filters"

