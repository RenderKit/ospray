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

#include <array>
#include "ospcommon/vec.h"
#include "ospray_test_fixture.h"

using OSPRayTestScenes::Base;
using OSPRayTestScenes::SingleObject;
using OSPRayTestScenes::Box;
using OSPRayTestScenes::Sierpinski;
using OSPRayTestScenes::Pipes;
using namespace ospcommon;

namespace {

OSPGeometry getMesh() {
  // triangle mesh data
  std::array<vec3f, 4> vertices = {
    vec3f(-2.0f, -1.0f, 3.5f),
    vec3f(2.0f, -1.0f, 3.0f),
    vec3f(2.0f,  1.0f, 3.5f),
    vec3f(-2.0f,  1.0f, 3.0f)
  };
  // triangle color data
  std::array<vec4f, 4> color =  {
    vec4f(1.0f, 0.0f, 0.0f, 1.0f),
    vec4f(0.0f, 1.0f, 0.0f, 1.0f),
    vec4f(0.0f, 0.0f, 1.0f, 1.0f),
    vec4f(0.7f, 0.7f, 0.7f, 1.0f)
  };
  std::array<vec3i, 4> indices = {
    vec3i(0, 1, 2),
    vec3i(0, 1, 3),
    vec3i(0, 2, 3),
    vec3i(1, 2, 3)
  };
  // create and setup model and mesh
  OSPGeometry mesh = ospNewGeometry("triangles");
  EXPECT_TRUE(mesh);
  OSPData data = ospNewData(vertices.size(), OSP_FLOAT3, vertices.data());
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(mesh, "vertex", data);
  data = ospNewData(color.size(), OSP_FLOAT4, color.data());
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(mesh, "vertex.color", data);
  data = ospNewData(indices.size(), OSP_INT3, indices.data());
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(mesh, "index", data);
  ospCommit(mesh);

  return mesh;
}

OSPGeometry getCylinder() {
  // cylinder vertex data
  std::array<vec3f, 2> vertex = {
    vec3f(0.0f, -1.0f, 3.0f),
    vec3f(0.0f, 1.0f, 3.0f)
  };
  // cylinder color
  vec4f color(1.0f, 0.0f, 0.0f, 1.0f);
  // create and setup model and cylinder
  OSPGeometry cylinder = ospNewGeometry("cylinders");
  EXPECT_TRUE(cylinder);
  OSPData data = ospNewData(vertex.size(), OSP_FLOAT3, vertex.data());
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(cylinder, "cylinders", data);
  data = ospNewData(1, OSP_FLOAT4, &color);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(cylinder, "color", data);
  ospSet1f(cylinder, "radius", 1.0f);
  ospCommit(cylinder);

  return cylinder;
}

OSPGeometry getSphere() {
  // sphere vertex data
  vec4f vertex(0.0f, 0.0f, 3.0f, 0.0f);
  vec4f color(1.0f, 0.0f, 0.0f, 1.0f);
  // create and setup model and sphere
  OSPGeometry sphere = ospNewGeometry("spheres");
  EXPECT_TRUE(sphere);
  OSPData data = ospNewData(1, OSP_FLOAT4, &vertex);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(sphere, "spheres", data);
  data = ospNewData(1, OSP_FLOAT4, &color);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(sphere, "color", data);
  ospSet1f(sphere, "radius", 1.0f);
  ospCommit(sphere);

  return sphere;
}

OSPGeometry getSphereColorFloat3() {
  // sphere vertex data
  vec4f vertex(0.0f, 0.0f, 3.0f, 0.0f);
  vec3f color(1.0f, 0.0f, 0.0f);
  // create and setup model and sphere
  OSPGeometry sphere = ospNewGeometry("spheres");
  EXPECT_TRUE(sphere);
  OSPData data = ospNewData(1, OSP_FLOAT4, &vertex);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(sphere, "spheres", data);
  data = ospNewData(1, OSP_FLOAT3, &color);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(sphere, "color", data);
  ospSet1f(sphere, "radius", 1.0f);
  ospCommit(sphere);

  return sphere;
}

OSPGeometry getSphereColorUchar4() {
  // sphere vertex data
  vec4f vertex(0.0f, 0.0f, 3.0f, 0.0f);
  vec4uc color(255, 0, 0, 255);
  // create and setup model and sphere
  OSPGeometry sphere = ospNewGeometry("spheres");
  EXPECT_TRUE(sphere);
  OSPData data = ospNewData(1, OSP_FLOAT4, &vertex);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(sphere, "spheres", data);
  data = ospNewData(1, OSP_UCHAR4, &color);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(sphere, "color", data);
  ospSet1f(sphere, "radius", 1.0f);
  ospCommit(sphere);

  return sphere;
}

OSPGeometry getSphereInterleavedLayout() {
  struct Sphere {
    vec3f vertex;
    vec4f color;

    Sphere() : vertex(0), color(0) {}
  };

  Sphere s;
  s.vertex.z = 3;
  s.color.x = 1;
  s.color.w = 1;

  // create and setup model and sphere
  OSPGeometry sphere = ospNewGeometry("spheres");
  EXPECT_TRUE(sphere);
  OSPData data = ospNewData(sizeof(Sphere), OSP_CHAR, &s);
  EXPECT_TRUE(data);
  ospCommit(data);
  ospSetData(sphere, "spheres", data);
  ospSetData(sphere, "color", data);
  ospSet1i(sphere, "color_offset", sizeof(vec3f));
  ospSet1i(sphere, "bytes_per_sphere", sizeof(Sphere));
  ospSet1i(sphere, "color_stride", sizeof(Sphere));
  ospSet1i(sphere, "color_format", OSP_FLOAT4);
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

// single red sphere with OSP_FLOAT3 color layout
TEST_P(SingleObject, simpleSphereColorFloat3) {
  OSPGeometry sphere = ::getSphereColorFloat3();
  ospSetMaterial(sphere, GetMaterial());
  ospCommit(sphere);
  AddGeometry(sphere);
  PerformRenderTest();
}

// single red sphere with OSP_UCHAR4 color layout
TEST_P(SingleObject, simpleSphereColorUchar4) {
  OSPGeometry sphere = ::getSphereColorUchar4();
  ospSetMaterial(sphere, GetMaterial());
  ospCommit(sphere);
  AddGeometry(sphere);
  PerformRenderTest();
}

// single red sphere with interleaved layout
TEST_P(SingleObject, simpleSphereInterleavedLayout) {
  OSPGeometry sphere = ::getSphereInterleavedLayout();
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

#if 0 // Broken PT tests...image diffs changed too much...
INSTANTIATE_TEST_CASE_P(MaterialPairs, Box, ::testing::Combine(::testing::Values("OBJMaterial", "Glass", "Luminous"), ::testing::Values("OBJMaterial", "Glass", "Luminous")));
#endif

TEST_P(Pipes, simple) {
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Scivis, Pipes, ::testing::Combine(::testing::Values("scivis"), ::testing::Values("OBJMaterial"), ::testing::Values(0.1f, 0.4f)));

// Tests disabled due to issues for pathtracer renderer with streamlines
#if 0 // TODO: these tests will break future tests...
INSTANTIATE_TEST_CASE_P(Pathtracer, Pipes, ::testing::Combine(::testing::Values("pathtracer"), ::testing::Values("OBJMaterial", "Glass", "Luminous"), ::testing::Values(0.1f, 0.4f)));
#endif
