// Copyright 2017-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"

using OSPRayTestScenes::FromOsprayTesting;
using OSPRayTestScenes::SpherePrecision;

using namespace ospcommon;

TEST_P(SpherePrecision, sphere)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Intersection,
    SpherePrecision,
    ::testing::Combine(::testing::Values(0.001f, 3.e5f),
        ::testing::Values(3.f, 8000.0f, 2.e5f),
        ::testing::Values(true, false),
        ::testing::Values("scivis", "pathtracer")));

TEST_P(FromOsprayTesting, test_scenes)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(TestScenesGeometry,
    FromOsprayTesting,
    ::testing::Combine(::testing::Values("cornell_box",
                           "curves",
                           "gravity_spheres_isosurface",
                           "empty",
                           "random_spheres",
                           "streamlines",
                           "subdivision_cube",
                           "cornell_box_photometric"),
        ::testing::Values("scivis", "pathtracer")));
