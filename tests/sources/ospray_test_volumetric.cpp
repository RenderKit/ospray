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

using OSPRayTestScenes::Sierpinski;
using OSPRayTestScenes::Torus;
using OSPRayTestScenes::TextureVolume;
using OSPRayTestScenes::DepthCompositeVolume;

TEST_P(Sierpinski, simple) {
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Volumetric, Sierpinski, ::testing::Combine(::testing::Values("scivis"), ::testing::Values(false), ::testing::Values(4, 6, 9)));

#if 0 // NOTE(jda) - enabling the pathtracer in the following tests breaks the Torus tests (!?!?!?!)
INSTANTIATE_TEST_CASE_P(Isosurfaces, Sierpinski, ::testing::Combine(::testing::Values("scivis", "pathtracer"), ::testing::Values(true), ::testing::Values( 5, 7,  9)));
#else
INSTANTIATE_TEST_CASE_P(Isosurfaces, Sierpinski, ::testing::Combine(::testing::Values("scivis"), ::testing::Values(true), ::testing::Values( 5, 7,  9)));
#endif

TEST_P(Torus, simple) {
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Renderers, Torus, ::testing::Values("scivis", "pathtracer"));

TEST_P(TextureVolume, simple) {
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Renderers, TextureVolume, ::testing::Values("scivis"));

TEST_P(DepthCompositeVolume, simple) {
 OSPTexture2D depthTex = ospNewTexture("texture2D");
 osp::vec2i size = imgSize;
 float* data = new float[size.x*size.y];
 for (size_t y = 0; y < size.y; y++) {
   for (size_t x = 0; x < size.x; x++) {
     size_t index = size.y*y + x;
     if (x < size.x/3) {
       data[index] = 999.f;
     } else if (x < (size.x*2)/3) {
       data[index] = 0.1f;
     } else {
       data[index] = 0.00001f;
     }
   }
 }
 auto ospData = ospNewData(size.x*size.y, OSP_FLOAT, data);
 ospSet1i(depthTex, "type", (int)OSP_TEXTURE_R32F);
 ospSet1i(depthTex, "flags", 0);
 ospSet2i(depthTex, "size", size.x, size.y);
 ospSetObject(depthTex, "data", ospData);
 PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Renderers, DepthCompositeVolume, ::testing::Values("scivis"));
