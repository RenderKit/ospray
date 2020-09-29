// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_light.h"
#include "ospray_testing.h"

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

GeometricLight::GeometricLight()
{
  rendererType = "pathtracer";
  samplesPerPixel = 16;
}

void GeometricLight::SetUp()
{
  Base::SetUp();

  // get test parameter
  auto params = GetParam();
  float size = std::get<0>(params);
  bool useMaterialList = std::get<1>(params);

  // build cornell box scene
  auto builder = ospray::testing::newBuilder("cornell_box");
  ospray::testing::setParam(builder, "rendererType", rendererType);
  ospray::testing::commit(builder);
  auto group = ospray::testing::buildGroup(builder);
  AddInstance(cpp::Instance(group));
  ospray::testing::release(builder);

  // build light
  float area = size * size;
  float halfSize = 0.5f * size;
  std::vector<vec3f> lightVertices = {{-halfSize, 0.98f, -halfSize},
      {halfSize, 0.98f, -halfSize},
      {halfSize, 0.98f, halfSize},
      {-halfSize, 0.98f, halfSize}};
  std::vector<vec4ui> lightIndices = {{0, 1, 2, 3}};

  cpp::Geometry lightMesh("mesh");
  lightMesh.setParam("vertex.position", cpp::CopiedData(lightVertices));
  lightMesh.setParam("index", cpp::CopiedData(lightIndices));
  lightMesh.commit();
  cpp::GeometricModel lightModel(lightMesh);

  cpp::Material lightMaterial(rendererType, "luminous");
  lightMaterial.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  lightMaterial.setParam("intensity", 10.f / area);
  lightMaterial.commit();

  if (useMaterialList) {
    std::vector<cpp::Material> materials{lightMaterial};
    renderer.setParam("material", cpp::CopiedData(materials));
    uint32_t materialIndex = 0;
    lightModel.setParam("material", materialIndex);
  } else {
    lightModel.setParam("material", lightMaterial);
  }

  AddModel(lightModel);

  camera.setParam("position", vec3f(0, 0, -2.4641));
  renderer.setParam("maxPathLength", 1);
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

TEST_P(GeometricLight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(LightingGeometric,
    GeometricLight,
    ::testing::Combine(
        ::testing::Values(0.2f, 0.4f), ::testing::Values(false, true)));

} // namespace OSPRayTestScenes
