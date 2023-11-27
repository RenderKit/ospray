// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ospray_testing.h"
#include "test_fixture.h"

namespace OSPRayTestScenes {

// Test all stereo modes of perspective and panoramic
class Stereo : public Base,
               public ::testing::TestWithParam<
                   std::tuple<const char * /*camera*/, OSPStereoMode>>
{
 public:
  Stereo();
  void SetUp() override;

 protected:
  std::string cameraType;
  vec3f pos{0.0f, 0.0f, -0.5f};
  OSPStereoMode stereoMode;
};

class PixelFilter
    : public Base,
      public ::testing::TestWithParam<
          std::tuple<const char * /*renderer*/, OSPPixelFilterType>>
{
 public:
  PixelFilter();
  void SetUp() override;

 protected:
  OSPPixelFilterType pixelFilter;
};

// Implementations /////////////////////////////////////////////////////////////

Stereo::Stereo()
{
  rendererType = "scivis";
  samplesPerPixel = 4;

  auto params = GetParam();
  cameraType = std::get<0>(params);
  stereoMode = std::get<1>(params);
}

void Stereo::SetUp()
{
  Base::SetUp();

  auto builder = ospray::testing::newBuilder("cornell_box");
  ospray::testing::setParam(builder, "rendererType", rendererType);
  ospray::testing::commit(builder);

  world = ospray::testing::buildWorld(builder);
  ospray::testing::release(builder);

  camera = cpp::Camera(cameraType);
  if (cameraType == "perspective") {
    pos.z = -2.f;
    camera.setParam("aspect", imgSize.x / (float)imgSize.y);
  }
  camera.setParam("position", pos);
  camera.setParam("stereoMode", stereoMode);
  camera.setParam("transform", affine3f(one));
}

PixelFilter::PixelFilter()
{
  auto params = GetParam();
  rendererType = std::get<0>(params);
  pixelFilter = std::get<1>(params);
}

void PixelFilter::SetUp()
{
  Base::SetUp();

  const float aspect = imgSize.x / (float)imgSize.y;
  const float diag = sqrt(aspect * aspect + 1.f);

  for (float r = 0.1f; r < diag;) {
    const float w = lerp(sqrt(r / diag), 0.1f, 0.002f);
    cpp::Light light("spot");
    light.setParam("innerRadius", r);
    light.setParam("radius", r + 0.5f * w);
    light.setParam("intensityQuantity", OSP_INTENSITY_QUANTITY_RADIANCE);
    AddLight(light);
    r += w;
  }

  camera = cpp::Camera("orthographic");
  camera.setParam("aspect", aspect);
  camera.setParam("position", vec3f(0.5f * aspect, -0.5f, 1.f));
  camera.setParam("direction", vec3f(0.f, 0.f, -1.f));

  renderer.setParam("pixelFilter", pixelFilter);
  renderer.setParam("backgroundColor", vec4f(0.f, 0.f, 0.f, 1.0f));
  if (rendererType == "scivis")
    renderer.setParam("visibleLights", true);
}

// Test Instantiations /////////////////////////////////////////////////////////

TEST_P(Stereo, stereo)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Camera,
    Stereo,
    ::testing::Combine(::testing::Values("perspective", "panoramic"),
        ::testing::Values(OSP_STEREO_LEFT,
            OSP_STEREO_RIGHT,
            OSP_STEREO_SIDE_BY_SIDE,
            OSP_STEREO_TOP_BOTTOM)));

TEST_P(PixelFilter, test)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Camera,
    PixelFilter,
    ::testing::Combine(::testing::Values("scivis", "pathtracer"),
        ::testing::Values(OSP_PIXELFILTER_POINT,
            OSP_PIXELFILTER_BOX,
            OSP_PIXELFILTER_GAUSS,
            OSP_PIXELFILTER_MITCHELL,
            OSP_PIXELFILTER_BLACKMAN_HARRIS)));
} // namespace OSPRayTestScenes
