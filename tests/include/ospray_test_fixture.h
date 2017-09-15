#pragma once

#include <string>
#include <vector>
#include <tuple>

#include <ospray/ospray.h>
#include <gtest/gtest.h>

#include "ospray_environment.h"
#include "ospray_test_tools.h"

namespace OSPRayTestScenes {

class Base {
 protected:
  osp::vec2i imgSize;
  std::string testName;
  std::string rendererType;
  int frames;

  OSPFrameBuffer framebuffer;
  OSPRenderer renderer;
  OSPCamera camera;
  OSPModel world;
  OSPData lights;

  std::vector<OSPLight> lightsList;

public:
  Base();
  ~Base();

  virtual void SetUp();
  virtual void TearDown();

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

  void RenderFrame(const uint32_t frameBufferChannels = OSP_FB_COLOR | OSP_FB_ACCUM);
};

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

class Sierpinski : public Base, public ::testing::TestWithParam<std::tuple<const char*, bool, int>> {
protected:
  int level;
  bool renderIsosurface;
public:
  Sierpinski();
  virtual void SetUp();
};

class Torus : public Base, public ::testing::TestWithParam<const char*> {
public:
  Torus();
  virtual void SetUp();
};

} // namespace OSPRayTestScenes

