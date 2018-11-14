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

#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <cmath>
#include <cstring>

#include <ospray/ospray.h>
#include <gtest/gtest.h>

#include "ospray_environment.h"
#include "ospray_test_tools.h"

namespace OSPRayTestScenes {

// Base class for all test fixtures.
// Deriving classes can call CreateEmptyScene() to set up model, renderer, camera etc.
// Behaviour of this method can be changed by modifying fields rendererType, frames and
// samplesPerPixel beforehand.
class Base {
 protected:
  osp::vec2i imgSize;
  std::string testName;
  std::string rendererType;
  int frames;
  int samplesPerPixel;

  OSPFrameBuffer framebuffer;
  OSPRenderer renderer;
  OSPCamera camera;
  OSPModel world;
  OSPData lights;
  OSPFrameBufferFormat frameBufferFormat = OSP_FB_SRGBA;

  OSPImageTools *imageTool;
  std::vector<OSPLight> lightsList;

public:
  Base();
  ~Base();

  virtual void SetUp();
  Base& operator=(const Base&) = delete;
  Base(const Base&) = delete;

  void AddLight(OSPLight new_light);
  void AddGeometry(OSPGeometry new_geometry);
  void AddVolume(OSPVolume new_volume);

  void PerformRenderTest();

  osp::vec2i GetImgSize() const { return imgSize; }
  std::string GetTestName() const { return testName; }

protected:
  void CreateEmptyScene();
  void SetCamera();
  void SetWorld();
  void SetLights();
  void SetRenderer();
  void SetFramebuffer();
  void SetImageTool();

  OSPMaterial CreateMaterial(std::string type);

  void RenderFrame(const uint32_t frameBufferChannels = OSP_FB_COLOR | OSP_FB_ACCUM);
};

// Fixture class for tests parametrized with renderer type and material type, intended for
// rendering scenes with a single object.
class SingleObject : public Base, public ::testing::TestWithParam<std::tuple<const char*, const char*>> {
protected:
  std::string materialType;
  OSPMaterial material;

public:
  SingleObject();
  virtual void SetUp();
  OSPMaterial GetMaterial() const { return material; }

protected:
  void SetMaterial();
};

// Fixture class that renders a fixed scene depicting a Cornell Box with a cuboid and a sphere.
// It is parametrized with two types of materials.
class Box : public Base, public ::testing::TestWithParam<std::tuple<const char*, const char*>> {
protected:
  std::string cuboidMaterialType;
  std::string sphereMaterialType;
  OSPMaterial cuboidMaterial;
  OSPMaterial sphereMaterial;

public:
  Box();
  virtual void SetUp();

protected:
  void SetMaterials();
  OSPMaterial GetMaterial(std::string);
};

// Fixture class that renders a Sierpinski tetrahedron using volumes or isosurfaces. It is
// parametrized with renderer type, boolean value that controls wheter volumes of isosurfaces
// should be used (false and true respectively) and number of steps taken to generate the fractal.
class Sierpinski : public Base, public ::testing::TestWithParam<std::tuple<const char*, bool, int>> {
protected:
  int level;
  bool renderIsosurface;
public:
  Sierpinski();
  virtual void SetUp();
private:
  std::vector<unsigned char> volumetricData;
};

// Fixture class used for tests that generates two isosurfaces, one in shape of a torus.
// It's parametrized with type of the renderer.
class Torus : public Base, public ::testing::TestWithParam<const char*> {
public:
  Torus();
  virtual void SetUp();
private:
  std::vector<float> volumetricData;
};

// Fixture for test that renders three cuts of a cubic volume.
class SlicedCube : public Base, public ::testing::Test {
public:
  SlicedCube();
  virtual void SetUp();
private:
  std::vector<float> volumetricData;
};

// Fixture class for testing materials of type "OBJMaterial". The rendered scene is composed of
// two quads and a luminous sphere. Parameters of this tests are passed to a new "OBJMaterial"
// material as "Kd", "Ks", "Ns", "d" and "Tf" and said material is used by the quads.
class MTLMirrors : public Base, public ::testing::TestWithParam<std::tuple<osp::vec3f, osp::vec3f, float, float, osp::vec3f>> {
public:
  MTLMirrors();
  virtual void SetUp();
private:
  osp::vec3f Kd;
  osp::vec3f Ks;
  float Ns;
  float d;
  osp::vec3f Tf;
};

// Fixture for tests rendering few connected cylinder segments. It's parametrized with type of
// material used and radius of the segments.
class Pipes : public Base, public ::testing::TestWithParam<std::tuple<const char*, const char*, float>> {
public:
  Pipes();
  virtual void SetUp();
private:
  float radius;
  std::string materialType;
};

// Test a texture colored by a volume.  Creates a sphere colored by the torus volume
// It's parametrized with type of the renderer.
class TextureVolume : public Base, public ::testing::TestWithParam<const char*> {
public:
  TextureVolume();
  virtual void SetUp();
private:
  std::vector<float> volumetricData;
};

} // namespace OSPRayTestScenes

