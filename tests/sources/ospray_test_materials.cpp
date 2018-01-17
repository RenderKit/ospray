// ======================================================================== //
// Copyright 2017-2018 Intel Corporation                                    //
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

using OSPRayTestScenes::MTLMirrors;

TEST_P(MTLMirrors, Properties) {
  PerformRenderTest();
}

// the quads are mirror-like and you can see the reflection of the sphere
INSTANTIATE_TEST_CASE_P(Shiny, MTLMirrors, ::testing::Combine(
  ::testing::Values(osp::vec3f({1.f, 1.f, 1.f}), osp::vec3f({1.f, 0.f, 0.f}),
  ::testing::Values(osp::vec3f({1.f, 1.f, 1.f}), osp::vec3f({1.f, 1.f, 0.f}),
  ::testing::Values(1000.f, 1000000.f),
  ::testing::Values(1.f),
  ::testing::Values(osp::vec3f({0.f, 0.f, 0.f}))
));

// the quads are slightly transparent
INSTANTIATE_TEST_CASE_P(Transparent, MTLMirrors, ::testing::Combine(
  ::testing::Values(osp::vec3f({1.f, 1.f, 1.f})),
  ::testing::Values(osp::vec3f({0.f, 0.f, 0.f})),
  ::testing::Values(1.f),
  ::testing::Values(0.9, 0.99f),
  ::testing::Values(osp::vec3f({0.f, 0.f, 0.f}), osp::vec3f({1.f, 0.f, 0.f})
));

// the quads use diffuse material
INSTANTIATE_TEST_CASE_P(Diffuse, MTLMirrors, ::testing::Combine(
  ::testing::Values(osp::vec3f({1.f, 1.f, 1.f}), osp::vec3f({1.f, 1.f, 0.f})),
  ::testing::Values(osp::vec3f({0.4f, 0.4f, 0.4f})),
  ::testing::Values(1.f),
  ::testing::Values(1.f),
  ::testing::Values(osp::vec3f({0.f, 0.f, 0.f}))
));

