// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_camera.h"
#include "ospray_testing.h"

namespace OSPRayTestScenes {

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

  instances.clear();

  auto builder = ospray::testing::newBuilder("cornell_box");
  ospray::testing::setParam(builder, "rendererType", rendererType);
  ospray::testing::commit(builder);

  world = ospray::testing::buildWorld(builder);
  ospray::testing::release(builder);
  world.commit();

  camera = cpp::Camera(cameraType);
  if (cameraType == "perspective") {
    pos.z = -2.f;
    camera.setParam("aspect", imgSize.x / (float)imgSize.y);
  }
  camera.setParam("position", pos);
  camera.setParam("stereoMode", stereoMode);
  camera.commit();
}

// Test Instantiations //////////////////////////////////////////////////////

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

} // namespace OSPRayTestScenes
