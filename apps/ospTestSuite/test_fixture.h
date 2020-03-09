// Copyright 2017-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>

#include "environment.h"
#include "test_tools.h"

#include "ospcommon/math/AffineSpace.h"

namespace OSPRayTestScenes {

// Base class for all test fixtures.
// Deriving classes can call CreateEmptyScene() to set up model, renderer,
// camera etc. Behaviour of this method can be changed by modifying fields
// rendererType, frames and samplesPerPixel beforehand.
class Base
{
 public:
  Base();
  virtual ~Base() = default;

  virtual void SetUp();
  Base &operator=(const Base &) = delete;
  Base(const Base &) = delete;

  void AddLight(cpp::Light new_light);
  void AddModel(cpp::GeometricModel model, affine3f xfm = one);
  void AddModel(cpp::VolumetricModel model, affine3f xfm = one);
  void AddInstance(cpp::Instance instance);

  void PerformRenderTest();

  vec2i GetImgSize() const
  {
    return imgSize;
  }
  std::string GetTestName() const
  {
    return testName;
  }

 protected:
  void CreateEmptyScene();
  void SetLights();

  void RenderFrame();

  // Data //

  vec2i imgSize;
  std::string testName;
  std::string rendererType;
  int frames;
  int samplesPerPixel;

  cpp::FrameBuffer framebuffer{nullptr};
  cpp::Renderer renderer{nullptr};
  cpp::Camera camera{nullptr};
  cpp::World world{nullptr};
  OSPFrameBufferFormat frameBufferFormat = OSP_FB_SRGBA;

  std::unique_ptr<OSPImageTools> imageTool;
  std::vector<cpp::Light> lightsList;

  std::vector<cpp::Instance> instances;
};

// Fixture class used for tests that uses 'ospray_testing' scenes
class FromOsprayTesting
    : public Base,
      public ::testing::TestWithParam<std::tuple<const char *, // scene name
          const char *>> // renderer
                         // type
{
 public:
  FromOsprayTesting();
  void SetUp() override;

 private:
  std::string sceneName;
};

} // namespace OSPRayTestScenes
