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

#include "ospray_test_fixture.h"

using OSPRayTestScenes::DepthCompositeVolume;
using OSPRayTestScenes::Sierpinski;
using OSPRayTestScenes::TextureVolume;
using OSPRayTestScenes::Torus;

TEST_P(Sierpinski, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Volumetric,
                        Sierpinski,
                        ::testing::Combine(::testing::Values("scivis"),
                                           ::testing::Values(false),
                                           ::testing::Values(4, 6, 9)));

INSTANTIATE_TEST_CASE_P(Isosurfaces,
                        Sierpinski,
                        ::testing::Combine(::testing::Values("scivis"),
                                           ::testing::Values(true),
                                           ::testing::Values(5, 7, 9)));

TEST_P(Torus, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Renderers, Torus, ::testing::Values("scivis"));

TEST_P(TextureVolume, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Renderers,
                        TextureVolume,
                        ::testing::Values("scivis"));

TEST_P(DepthCompositeVolume, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Renderers,
                        DepthCompositeVolume,
                        ::testing::Combine(::testing::Values("scivis"),
                                           ::testing::Values(vec4f(0.f),
                                                             vec4f(1.f),
                                                             vec4f(0.f, 0.f,
                                                                   0.f, 1.f),
                                                             vec4f(1.f, 0.f,
                                                                   0.f, 0.5f))));
