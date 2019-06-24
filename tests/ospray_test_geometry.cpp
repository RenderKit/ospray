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

#include <array>
#include "ospcommon/vec.h"
#include "ospray_test_fixture.h"

using OSPRayTestScenes::Base;
using OSPRayTestScenes::Box;
using OSPRayTestScenes::Pipes;
using OSPRayTestScenes::Sierpinski;
using OSPRayTestScenes::SingleObject;
using OSPRayTestScenes::SpherePrecision;
using OSPRayTestScenes::Subdivision;
using namespace ospcommon;

namespace {

  OSPGeometry getMesh()
  {
    // triangle mesh data
    std::array<vec3f, 4> vertices = {vec3f(-2.0f, -1.0f, 3.5f),
                                     vec3f(2.0f, -1.0f, 3.0f),
                                     vec3f(2.0f, 1.0f, 3.5f),
                                     vec3f(-2.0f, 1.0f, 3.0f)};
    // triangle color data
    std::array<vec4f, 4> color   = {vec4f(1.0f, 0.0f, 0.0f, 1.0f),
                                  vec4f(0.0f, 1.0f, 0.0f, 1.0f),
                                  vec4f(0.0f, 0.0f, 1.0f, 1.0f),
                                  vec4f(0.7f, 0.7f, 0.7f, 1.0f)};
    std::array<vec3i, 4> indices = {
        vec3i(0, 1, 2), vec3i(0, 1, 3), vec3i(0, 2, 3), vec3i(1, 2, 3)};
    // create and setup model and mesh
    OSPGeometry mesh = ospNewGeometry("triangles");
    EXPECT_TRUE(mesh);
    OSPData data = ospNewData(vertices.size(), OSP_VEC3F, vertices.data());
    EXPECT_TRUE(data);
    ospCommit(data);
    ospSetData(mesh, "vertex", data);
    ospRelease(data);
    data = ospNewData(color.size(), OSP_VEC4F, color.data());
    EXPECT_TRUE(data);
    ospCommit(data);
    ospSetData(mesh, "vertex.color", data);
    ospRelease(data);
    data = ospNewData(indices.size(), OSP_VEC3I, indices.data());
    EXPECT_TRUE(data);
    ospCommit(data);
    ospSetData(mesh, "index", data);
    ospRelease(data);
    ospCommit(mesh);

    return mesh;
  }

  OSPGeometry getCylinder()
  {
    // cylinder vertex data
    std::array<vec3f, 2> vertex = {vec3f(0.0f, -1.0f, 3.0f),
                                   vec3f(0.0f, 1.0f, 3.0f)};
    // create and setup model and cylinder
    OSPGeometry cylinder = ospNewGeometry("cylinders");
    EXPECT_TRUE(cylinder);
    OSPData data = ospNewData(vertex.size(), OSP_VEC3F, vertex.data());
    EXPECT_TRUE(data);
    ospCommit(data);
    ospSetData(cylinder, "cylinders", data);
    ospRelease(data);
    ospSetFloat(cylinder, "radius", 1.0f);
    ospCommit(cylinder);

    return cylinder;
  }

  OSPGeometry getSphere()
  {
    // sphere vertex data
    vec4f vertex(0.0f, 0.0f, 3.0f, 0.0f);
    vec4f color(1.0f, 0.0f, 0.0f, 1.0f);
    // create and setup model and sphere
    OSPGeometry sphere = ospNewGeometry("spheres");
    EXPECT_TRUE(sphere);
    OSPData data = ospNewData(1, OSP_VEC4F, &vertex);
    EXPECT_TRUE(data);
    ospCommit(data);
    ospSetData(sphere, "spheres", data);
    ospRelease(data);
    data = ospNewData(1, OSP_VEC4F, &color);
    EXPECT_TRUE(data);
    ospCommit(data);
    ospSetData(sphere, "color", data);
    ospRelease(data);
    ospSetFloat(sphere, "radius", 1.0f);
    ospCommit(sphere);

    return sphere;
  }

  OSPGeometry getStreamline(bool constantRadius = true)
  {
    float vertex[] = {2.0f,
                      -1.0f,
                      3.0f,
                      0.0f,
                      0.4f,
                      0.5f,
                      8.0f,
                      -2.0f,
                      0.0f,
                      1.0f,
                      3.0f,
                      0.0f};
    float color[]  = {
        0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f};
    int index[]   = {0, 1};
    float radii[] = {.1f, .3f, .5f};

    OSPGeometry streamlines = ospNewGeometry("streamlines");
    EXPECT_TRUE(streamlines);
    OSPData data = ospNewData(3, OSP_VEC3FA, vertex);
    EXPECT_TRUE(data);
    ospCommit(data);
    ospSetData(streamlines, "vertex", data);
    ospRelease(data);
    if (!constantRadius) {
      data = ospNewData(3, OSP_FLOAT, radii);
      EXPECT_TRUE(data);
      ospCommit(data);
      ospSetData(streamlines, "vertex.radius", data);
      ospRelease(data);
    }
    data = ospNewData(3, OSP_VEC4F, color);
    EXPECT_TRUE(data);
    ospCommit(data);
    ospSetData(streamlines, "vertex.color", data);
    ospRelease(data);
    data = ospNewData(2, OSP_INT, index);
    EXPECT_TRUE(data);
    ospCommit(data);
    ospSetData(streamlines, "index", data);
    ospRelease(data);
    ospSetFloat(streamlines, "radius", 0.5f);
    ospCommit(streamlines);

    return streamlines;
  }

}  // anonymous namespace

// an empty scene
TEST_P(SingleObject, emptyScene)
{
  ospRelease(GetMaterial());
  PerformRenderTest();
}

// a simple mesh
TEST_P(SingleObject, simpleMesh)
{
  OSPGeometry mesh = ::getMesh();

  OSPGeometricModel model = ospNewGeometricModel(mesh);
  ospRelease(mesh);
  auto material = GetMaterial();
  ospSetObject(model, "material", material);
  ospRelease(material);
  AddModel(model);

  PerformRenderTest();
}

// single red sphere
TEST_P(SingleObject, simpleSphere)
{
  OSPGeometry sphere = ::getSphere();

  vec4f color(1.0f, 0.0f, 0.0f, 1.0f);
  OSPData data = ospNewData(1, OSP_VEC4F, &color);
  EXPECT_TRUE(data);
  ospCommit(data);

  OSPGeometricModel model = ospNewGeometricModel(sphere);
  ospRelease(sphere);
  auto material = GetMaterial();
  ospSetObject(model, "material", material);
  ospRelease(material);
  ospSetData(model, "color", data);
  ospRelease(data);
  AddModel(model);

  PerformRenderTest();
}

// single red cylinder
TEST_P(SingleObject, simpleCylinder)
{
  OSPGeometry cylinder = ::getCylinder();

  vec4f color(1.0f, 0.0f, 0.0f, 1.0f);
  OSPData data = ospNewData(1, OSP_VEC4F, &color);
  EXPECT_TRUE(data);
  ospCommit(data);

  OSPGeometricModel model = ospNewGeometricModel(cylinder);
  ospRelease(cylinder);
  auto material = GetMaterial();
  ospSetObject(model, "material", material);
  ospRelease(material);
  ospSetData(model, "color", data);
  ospRelease(data);
  AddModel(model);

  PerformRenderTest();
}

// single streamline
TEST_P(SingleObject, simpleStreamlines)
{
  OSPGeometry streamlines = ::getStreamline();

  OSPGeometricModel model = ospNewGeometricModel(streamlines);
  ospRelease(streamlines);
  auto material = GetMaterial();
  ospSetObject(model, "material", material);
  ospRelease(material);
  AddModel(model);

  PerformRenderTest();
}

TEST_P(SingleObject, simpleStreamlinesVariableRadii)
{
  OSPGeometry streamlines = ::getStreamline(false);

  OSPGeometricModel model = ospNewGeometricModel(streamlines);
  ospRelease(streamlines);
  auto material = GetMaterial();
  ospSetObject(model, "material", material);
  ospRelease(material);
  AddModel(model);

  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Scivis,
                        SingleObject,
                        ::testing::Combine(::testing::Values("scivis"),
                                           ::testing::Values("OBJMaterial")));

INSTANTIATE_TEST_CASE_P(
    Pathtracer,
    SingleObject,
    ::testing::Combine(::testing::Values("pathtracer"),
                       ::testing::Values("OBJMaterial", "Glass", "Luminous")));

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

TEST_P(Box, basicScene)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(
    MaterialPairs,
    Box,
    ::testing::Combine(::testing::Values("OBJMaterial", "Glass", "Luminous"),
                       ::testing::Values("OBJMaterial", "Glass", "Luminous")));

TEST_P(Pipes, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Scivis,
                        Pipes,
                        ::testing::Combine(::testing::Values("scivis"),
                                           ::testing::Values("OBJMaterial"),
                                           ::testing::Values(0.1f, 0.4f)));
INSTANTIATE_TEST_CASE_P(Pathtracer,
                        Pipes,
                        ::testing::Combine(::testing::Values("pathtracer"),
                                           ::testing::Values("OBJMaterial",
                                                             "Luminous"),
                                           ::testing::Values(0.1f, 0.4f)));

TEST_P(Subdivision, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_CASE_P(Scivis,
                        Subdivision,
                        ::testing::Combine(::testing::Values("scivis"),
                                           ::testing::Values("OBJMaterial")));
INSTANTIATE_TEST_CASE_P(Pathtracer,
                        Subdivision,
                        ::testing::Combine(::testing::Values("pathtracer"),
                                           ::testing::Values("OBJMaterial")));
