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

using OSPRayTestScenes::DepthCompositeVolume;
using OSPRayTestScenes::HeterogeneousVolume;
using OSPRayTestScenes::TextureVolume;
using OSPRayTestScenes::Torus;

TEST_P(Torus, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Renderers, Torus, ::testing::Values("scivis"));

TEST_P(TextureVolume, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Renderers, TextureVolume, ::testing::Values("scivis"));

TEST_P(DepthCompositeVolume, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(
    Renderers,
    DepthCompositeVolume,
    ::testing::Combine(::testing::Values("scivis"),
                       ::testing::Values(vec4f(0.f),
                                         vec4f(1.f),
                                         vec4f(0.f, 0.f, 0.f, 1.f),
                                         vec4f(1.f, 0.f, 0.f, 0.5f))));

TEST_P(HeterogeneousVolume, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(
    VaryingDensity,
    HeterogeneousVolume,
    ::testing::Combine(::testing::Values(vec3f({1.0f, 1.0f, 1.0f})),
                       ::testing::Values(0.f),
                       ::testing::Values(0.2f, 25.f),
                       ::testing::Values(vec3f({0.03, 0.07, 0.23})),
                       ::testing::Values(true),
                       ::testing::Values(true),
                       ::testing::Values(true, false),
                       ::testing::Values(16)));

INSTANTIATE_TEST_CASE_P(
    VaryingAlbedo,
    HeterogeneousVolume,
    ::testing::Combine(::testing::Values(vec3f({0.0f, 0.0f, 0.0f}),
                                         vec3f({0.5f, 0.5f, 0.5f}),
                                         vec3f({0.8f, 0.2f, 0.8f})),
                       ::testing::Values(0.f),
                       ::testing::Values(1.f),
                       ::testing::Values(vec3f({0.03, 0.07, 0.23})),
                       ::testing::Values(true),
                       ::testing::Values(true),
                       ::testing::Values(true, false),
                       ::testing::Values(16)));

INSTANTIATE_TEST_CASE_P(
    VaryingAnisotropy,
    HeterogeneousVolume,
    ::testing::Combine(::testing::Values(vec3f({1.0f, 1.0f, 1.0f})),
                       ::testing::Values(-1.f, -0.5, 0.5f, 1.f),
                       ::testing::Values(1.f),
                       ::testing::Values(vec3f({0.03, 0.07, 0.23})),
                       ::testing::Values(true),
                       ::testing::Values(true),
                       ::testing::Values(true),
                       ::testing::Values(16)));

INSTANTIATE_TEST_CASE_P(
    WhiteFurnaceTest,
    HeterogeneousVolume,
    ::testing::Combine(::testing::Values(vec3f({1.0f, 1.0f, 1.0f})),
                       ::testing::Values(0.f),
                       ::testing::Values(1.f),
                       ::testing::Values(vec3f({1.0, 1.0, 1.0})),
                       ::testing::Values(false),
                       ::testing::Values(false),
                       ::testing::Values(true, false),
                       ::testing::Values(256)));
