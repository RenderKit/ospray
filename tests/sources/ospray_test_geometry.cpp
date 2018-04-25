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

using OSPRayTestScenes::Base;
using OSPRayTestScenes::SingleObject;
using OSPRayTestScenes::Box;
using OSPRayTestScenes::Sierpinski;
using OSPRayTestScenes::Pipes;

namespace {

OSPGeometry getMesh() {
  // triangle mesh data
  float vertices[] = { -2.0f, -1.0f, 3.5f,
                       2.0f, -1.0f, 3.0f,
                       2.0f,  1.0f, 3.5f,
                       -2.0f,  1.0f, 3.0f
                     };
  // triangle color data
  float color[] =  { 1.0f, 0.0f, 0.0f, 1.0f,
                     0.0f, 1.0f, 0.0f, 1.0f,
                     0.0f, 0.0f, 1.0f, 1.0f,
                     0.7f, 0.7f, 0.7f, 1.0f
                   };
  int32_t indices[] = { 0, 1, 2,
                        0, 1, 3,
                        0, 2, 3,
                        1, 2, 3
                      };
  // create and setup model and mesh
  OSPGeometry mesh = ospNewGeometry("triangles");
  EXPECT_TRUE(mesh);
  OSPData data = ospNewData(4, OSP_FLOAT3, vertices);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(mesh, "vertex", data);
  data = ospNewData(4, OSP_FLOAT4, color);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(mesh, "vertex.color", data);
  data = ospNewData(4, OSP_INT3, indices);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(mesh, "index", data);
  ospCommit(mesh);

  return mesh;
}

OSPGeometry getCylinder() {
  //  cylinder vertex data
  float vertex[] = { 0.0f, -1.0f, 3.0f,
                     0.0f, 1.0f, 3.0f
                   };
  //  cylinder color
  float color[] =  { 1.0f, 0.0f, 0.0f, 1.0f};
  // create and setup model and cylinder
  OSPGeometry cylinder = ospNewGeometry("cylinders");
  EXPECT_TRUE(cylinder);
  OSPData data = ospNewData(2, OSP_FLOAT3, vertex);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(cylinder, "cylinders", data);
  data = ospNewData(1, OSP_FLOAT4, color);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(cylinder, "color", data);
  ospSet1f(cylinder, "radius", 1.0f);
  ospCommit(cylinder);

  return cylinder;
}

OSPGeometry getSphere() {
  // sphere vertex data
  float vertex[] = { 0.0f, 0.0f, 3.0f, 0.0f};
  float color[] =  { 1.0f, 0.0f, 0.0f, 1.0f};
  // create and setup model and sphere
  OSPGeometry sphere = ospNewGeometry("spheres");
  EXPECT_TRUE(sphere);
  OSPData data = ospNewData(1, OSP_FLOAT4, vertex);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(sphere, "spheres", data);
  data = ospNewData(1, OSP_FLOAT4, color);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(sphere, "color", data);
  ospSet1f(sphere, "radius", 1.0f);
  ospCommit(sphere);

  return sphere;
}

OSPGeometry getStreamline() {
  float vertex[] = { 2.0f, -1.0f, 3.0f, 0.0f,
                     0.4f, 0.5f, 8.0f, -2.0f,
                     0.0f, 1.0f, 3.0f, 0.0f
                   };
  float color[] =  { 0.0f, 1.0f, 0.0f, 1.0f,
                     0.0f, 0.0f, 1.0f, 1.0f,
                     1.0f, 0.0f, 0.0f, 1.0f
                   };
  int index[] =  { 0, 1};
  OSPGeometry streamlines = ospNewGeometry("streamlines");
  EXPECT_TRUE(streamlines);
  OSPData data = ospNewData(3, OSP_FLOAT3A, vertex);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(streamlines, "vertex", data);
  data = ospNewData(3, OSP_FLOAT4, color);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(streamlines, "vertex.color", data);
  data = ospNewData(2, OSP_INT, index);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(streamlines, "index", data);
  ospSet1f(streamlines, "radius", 0.5f);
  ospCommit(streamlines);

  return streamlines;
}

} // anonymous namespace

// an empty scene
TEST_P(SingleObject, emptyScene) {
  PerformRenderTest();
}

// a simple mesh
TEST_P(SingleObject, simpleMesh) {
  OSPGeometry mesh = ::getMesh();
  ospSetMaterial(mesh, GetMaterial());
  ospCommit(mesh);
  AddGeometry(mesh);
  PerformRenderTest();
}

// single red sphere
TEST_P(SingleObject, simpleSphere) {
  OSPGeometry sphere = ::getSphere();
  ospSetMaterial(sphere, GetMaterial());
  ospCommit(sphere);
  AddGeometry(sphere);
  PerformRenderTest();
}

// single red cylinder
TEST_P(SingleObject, simpleCylinder) {
  OSPGeometry cylinder = ::getCylinder();
  ospSetMaterial(cylinder, GetMaterial());
  ospCommit(cylinder);
  AddGeometry(cylinder);
  PerformRenderTest();
}

// single streamline
TEST_P(SingleObject, simpleStreamlines) {
  OSPGeometry streamlines = ::getStreamline();
  ospSetMaterial(streamlines, GetMaterial());
  ospCommit(streamlines);
  AddGeometry(streamlines);
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Scivis, SingleObject, ::testing::Combine(::testing::Values("scivis"), ::testing::Values("OBJMaterial")));
INSTANTIATE_TEST_CASE_P(Pathtracer, SingleObject, ::testing::Combine(::testing::Values("pathtracer"), ::testing::Values("OBJMaterial", "Glass", "Luminous")));

TEST_P(Box, basicScene) {
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(MaterialPairs, Box, ::testing::Combine(::testing::Values("OBJMaterial", "Glass", "Luminous"), ::testing::Values("OBJMaterial", "Glass", "Luminous")));

TEST_P(Pipes, simple) {
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Scivis, Pipes, ::testing::Combine(::testing::Values("scivis"), ::testing::Values("OBJMaterial"), ::testing::Values(0.1f, 0.4f)));

// Tests disabled due to issues for pathtracer renderer with streamlines
INSTANTIATE_TEST_CASE_P(Pathtracer, Pipes, ::testing::Combine(::testing::Values("pathtracer"), ::testing::Values("OBJMaterial", "Glass", "Luminous"), ::testing::Values(0.1f, 0.4f)));
