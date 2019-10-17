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

  using ospcommon::math::affine3f;
  using ospcommon::math::one;

  // Base class for all test fixtures.
  // Deriving classes can call CreateEmptyScene() to set up model, renderer,
  // camera etc. Behaviour of this method can be changed by modifying fields
  // rendererType, frames and samplesPerPixel beforehand.
  class Base
  {
   protected:
    vec2i imgSize;
    std::string testName;
    std::string rendererType;
    int frames;
    int samplesPerPixel;

    OSPFrameBuffer framebuffer{nullptr};
    OSPRenderer renderer{nullptr};
    OSPCamera camera{nullptr};
    OSPWorld world{nullptr};
    OSPData lights{nullptr};
    OSPFrameBufferFormat frameBufferFormat = OSP_FB_SRGBA;

    std::unique_ptr<OSPImageTools> imageTool;
    std::vector<OSPLight> lightsList;

    std::vector<OSPInstance> instances;

   public:
    Base();
    virtual ~Base();

    virtual void SetUp();
    Base &operator=(const Base &) = delete;
    Base(const Base &)            = delete;

    void AddLight(OSPLight new_light);
    void AddModel(OSPGeometricModel model, affine3f xfm = one);
    void AddModel(OSPVolumetricModel model, affine3f xfm = one);
    void AddInstance(OSPInstance instance);

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
    void SetCamera();
    void SetWorld();
    void SetLights();
    void SetRenderer();
    void SetFramebuffer();
    void SetImageTool();

    OSPMaterial CreateMaterial(std::string type);

    void RenderFrame();
  };

  // Fixture class to test cornercases of intersection precision and epsilon
  // handling; parametrized with renderer, sphere radius, distance factor, and
  // whether the sphere is in origin
  // TODO generalize for other geometries as well, reusing SingleObject
  class SpherePrecision
      : public Base,
        public ::testing::TestWithParam<std::tuple<float /*radius*/,
                                                   float /*factor*/,
                                                   bool /*move_cam*/,
                                                   const char * /*renderer*/>>
  {
   public:
    SpherePrecision();
    virtual void SetUp();

   protected:
    float dist;
    float radius;
    bool move_cam;
  };

  // Fixture class used for tests that generates two isosurfaces, one in shape
  // of a torus. It's parametrized with type of the renderer.
  class Torus : public Base, public ::testing::TestWithParam<const char *>
  {
   public:
    Torus();
    virtual void SetUp();

   private:
    std::vector<float> volumetricData;
  };

  // Test a texture colored by a volume.  Creates a sphere colored by the torus
  // volume It's parametrized with type of the renderer.
  class TextureVolume : public Base,
                        public ::testing::TestWithParam<const char *>
  {
   public:
    TextureVolume();
    virtual void SetUp();

   private:
    std::vector<float> volumetricData;
  };

  // Test a texture colored by a volume.  Creates a sphere colored by the torus
  // volume It's parametrized with type of the renderer and background color
  class DepthCompositeVolume
      : public Base,
        public ::testing::TestWithParam<std::tuple<const char *, vec4f>>
  {
   public:
    DepthCompositeVolume();
    virtual void SetUp();

   private:
    std::vector<float> volumetricData;
    vec4f bgColor;
  };

  // Fixture for tests rendering a Subdivision mesh. It's parametrized with type
  // of material used.
  class Subdivision
      : public Base,
        public ::testing::TestWithParam<std::tuple<const char *, const char *>>
  {
   public:
    Subdivision();
    virtual void SetUp();

   private:
    std::string materialType;
  };

  // Fixture class that renders a simple heterogeneous volume using the
  // pathtracer. It is parametrized with albedo, anisotropy and density scale.
  class HeterogeneousVolume : public Base,
                              public ::testing::TestWithParam<
                                  std::tuple<vec3f,  // albedo
                                             float,  // anisotropy
                                             float,  // densityScale
                                             vec3f,  // color of ambient light
                                             bool,   // enable distant light
                                             bool,   // enable geometry
                                             bool,   // constant volume
                                             int>>   // samples per pixel
  {
   protected:
    vec3f albedo;
    float anisotropy;
    float densityScale;
    vec3f ambientColor;
    bool enableDistantLight;
    bool enableGeometry;
    bool constantVolume;

   public:
    HeterogeneousVolume();
    virtual void SetUp();
  };

}  // namespace OSPRayTestScenes
