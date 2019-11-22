// ======================================================================== //
// Copyright 2017-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "test_fixture.h"

using OSPRayTestScenes::FromOsprayTesting;
using OSPRayTestScenes::SpherePrecision;

using namespace ospcommon;

TEST_P(SpherePrecision, sphere)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(
    Intersection,
    SpherePrecision,
    ::testing::Combine(::testing::Values(0.001f, 3.e5f),
                       ::testing::Values(3.f, 8000.0f, 2.e5f),
                       ::testing::Values(true, false),
                       ::testing::Values("scivis", "pathtracer")));

TEST_P(FromOsprayTesting, test_scenes)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(
    TestScenesGeometry,
    FromOsprayTesting,
    ::testing::Combine(::testing::Values("cornell_box",
                                         "curves",
                                         "gravity_spheres_isosurface",
                                         "empty",
                                         "random_spheres",
                                         "streamlines",
                                         "subdivision_cube"),
                       ::testing::Values("scivis", "pathtracer")));

INSTANTIATE_TEST_CASE_P(
    TestScenesPtMaterials,
    FromOsprayTesting,
    ::testing::Combine(::testing::Values("test_pt_glass",
                                         "test_pt_luminous",
                                         "test_pt_metal_roughness",
                                         "test_pt_metallic_flakes",
                                         "test_pt_obj"),
                       ::testing::Values("pathtracer")));
