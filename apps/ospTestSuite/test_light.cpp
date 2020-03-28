// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_light.h"

namespace OSPRayTestScenes {

SunSky::SunSky()
{
  rendererType = "pathtracer";
  samplesPerPixel = 1;
}

void SunSky::SetUp()
{
  Base::SetUp();
  auto params = GetParam();

  cpp::Light light("sunSky");
  float turb = std::get<2>(params);
  light.setParam("up", std::get<0>(params));
  light.setParam("direction", std::get<1>(params));
  light.setParam("turbidity", turb);
  light.setParam("albedo", std::get<3>(params));
  // lower brightness with high turbidity
  light.setParam("intensity", 1.0f / turb);
  AddLight(light);

  renderer.setParam("backgroundColor", vec4f(0.f, 0.f, 0.f, 1.0f));
}

// Test Instantiations //////////////////////////////////////////////////////

TEST_P(SunSky, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Light,
    SunSky,
    ::testing::Combine(::testing::Values(vec3f(0.f, 0.8f, 0.4f)),
        ::testing::Values(vec3f(0.f, 0.7f, -1.f),
            vec3f(0.f, 0.4f, -1.f),
            vec3f(0.f, 0.1f, -1.f),
            vec3f(0.f, -0.3f, -1.f),
            vec3f(0.f, -0.8f, 0.4f)),
        ::testing::Values(1.0f, 3.0f, 10.0f),
        ::testing::Values(0.0f)));

INSTANTIATE_TEST_SUITE_P(Light2,
    SunSky,
    ::testing::Combine(::testing::Values(vec3f(0.2f, -0.5f, 0.f)),
        ::testing::Values(vec3f(0.2f, 0.4f, -1.f), vec3f(0.f, 0.f, -1.f)),
        ::testing::Values(2.0f),
        ::testing::Values(0.0f, 1.0f)));

} // namespace OSPRayTestScenes
