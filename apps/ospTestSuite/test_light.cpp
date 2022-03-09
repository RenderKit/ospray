// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_light.h"
#include "ospray_testing.h"

namespace OSPRayTestScenes {

LightTest::LightTest()
{
  samplesPerPixel = 16;
}

void LightTest::SetUp()
{
  Base::SetUp();

  // set common renderer parameter
  if (rendererType == "pathtracer")
    renderer.setParam("maxPathLength", 1);
  if (rendererType == "scivis") {
    renderer.setParam("shadows", true);
    renderer.setParam("visibleLights", true);
  }

  // build cornell box scene
  auto builder = ospray::testing::newBuilder("cornell_box");
  ospray::testing::setParam(builder, "rendererType", rendererType);
  ospray::testing::commit(builder);
  auto group = ospray::testing::buildGroup(builder);
  AddInstance(cpp::Instance(group));
  ospray::testing::release(builder);

  // position camera
  camera.setParam("position", vec3f(0, 0, -2.4641));
}

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
  light.setParam("intensityQuantity", OSP_INTENSITY_QUANTITY_SCALE);
  light.setParam("intensity", 0.025f / turb);
  light.setParam("horizonExtension", std::get<4>(params));
  AddLight(light);

  renderer.setParam("backgroundColor", vec4f(0.f, 0.f, 0.f, 1.0f));
}

AmbientLight::AmbientLight()
{
  rendererType = GetParam();
}

void AmbientLight::SetUp()
{
  LightTest::SetUp();

  cpp::Light ambient = ospNewLight("ambient");
  ambient.setParam("intensity", 0.5f);
  AddLight(ambient);

  if (rendererType == "scivis")
    renderer.setParam("aoSamples", 1);
}

DistantLight::DistantLight()
{
  auto params = GetParam();
  direction = std::get<0>(params);
  rendererType = std::get<1>(params);
}

void DistantLight::SetUp()
{
  LightTest::SetUp();

  cpp::Light distant("distant");
  distant.setParam("direction", direction);
  distant.setParam("color", vec3f(1.0f, 0.75f, 0.25f));
  distant.setParam("angularDiameter", 1.0f);
  AddLight(distant);
}

GeometricLight::GeometricLight()
{
  rendererType = "pathtracer";
  auto params = GetParam();
  size = std::get<0>(params);
  useMaterialList = std::get<1>(params);
}

void GeometricLight::SetUp()
{
  LightTest::SetUp();

  const float area = size * size;
  const float halfSize = 0.5f * size;
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
}

PhotometricLight::PhotometricLight()
{
  auto params = GetParam();
  radius = std::get<0>(params);
  rendererType = std::get<1>(params);
}

void PhotometricLight::SetUp()
{
  LightTest::SetUp();

  cpp::Light light1d("spot");
  light1d.setParam("intensity", 5.f);
  light1d.setParam("position", vec3f(-0.6f, 0.8f, -0.5f));
  light1d.setParam("direction", vec3f(0.0f, -1.0f, 0.0f));
  light1d.setParam("openingAngle", 360.f);
  light1d.setParam("penumbraAngle", 0.f);
  light1d.setParam("radius", radius);
  float lid1d[] = {2.5f, 0.4f, 0.2f, 0.1f, 0.03f, 0.01f, 0.01f};
  light1d.setParam("intensityDistribution", cpp::CopiedData(lid1d, 7));
  AddLight(light1d);

  cpp::Light light2d("spot");
  light2d.setParam("intensity", 1.f);
  light2d.setParam("position", vec3f(0.3f, 0.6f, 0.f));
  light2d.setParam("direction", vec3f(0.0f, -1.0f, 0.0f));
  light2d.setParam("openingAngle", 270.f);
  light2d.setParam("penumbraAngle", 10.f);
  light2d.setParam("radius", radius);
  float lid2d[60] = {
      1.5f, 5.0f, 6.0f, 0.3f, 0.01f, 0.15f, 0.5f, 1.6f, 0.1f, 0.01f};
  light2d.setParam(
      "intensityDistribution", cpp::CopiedData(lid2d, vec2ul(5, 12)));
  light2d.setParam("c0", vec3f(1.0f, 0.0f, 0.0f));
  AddLight(light2d);
}

QuadLight::QuadLight()
{
  auto params = GetParam();
  size = std::get<0>(params);
  rendererType = std::get<1>(params);
  intensityQuantity = std::get<2>(params);
}

void QuadLight::SetUp()
{
  LightTest::SetUp();

  cpp::Light light("quad");
  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 10.f);
  light.setParam("intensityQuantity", intensityQuantity);
  light.setParam("position", vec3f(size / -2.0f, 0.98f, size / -2.0f));
  light.setParam("edge1", vec3f(size, 0.0f, 0.0f));
  light.setParam("edge2", vec3f(0.0f, 0.0f, size));
  AddLight(light);
}

SphereLight::SphereLight()
{
  auto params = GetParam();
  radius = std::get<0>(params);
  rendererType = std::get<1>(params);
  intensityQuantity = std::get<2>(params);
}

void SphereLight::SetUp()
{
  LightTest::SetUp();

  cpp::Light light("sphere");
  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 2.5f);
  light.setParam("intensityQuantity", intensityQuantity);
  light.setParam("position", vec3f(0.0f, 0.48f, 0.0f));
  light.setParam("radius", radius);
  AddLight(light);
}

SpotLight::SpotLight()
{
  auto params = GetParam();
  innerOuterRadius = std::get<0>(params);
  rendererType = std::get<1>(params);
  intensityQuantity = std::get<2>(params);
}

void SpotLight::SetUp()
{
  LightTest::SetUp();

  cpp::Light light("spot");
  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 10.f);
  light.setParam("intensityQuantity", intensityQuantity);
  light.setParam("position", vec3f(0.0f, 0.98f, 0.0f));
  light.setParam("direction", vec3f(0.0f, -1.0f, 0.0f));
  light.setParam("radius", innerOuterRadius[1]);
  light.setParam("innerRadius", innerOuterRadius[0]);
  AddLight(light);
}

HDRILight::HDRILight()
{
  rendererType = GetParam();
}

void HDRILight::SetUp()
{
  Base::SetUp();

  // set common renderer parameter
  if (rendererType == "pathtracer")
    renderer.setParam("maxPathLength", 1);
  if (rendererType == "scivis") {
    renderer.setParam("shadows", true);
    renderer.setParam("visibleLights", true);
  }

  // create sphere
  cpp::Group group;
  {
    cpp::Geometry sphere("sphere");
    sphere.setParam("sphere.position", cpp::CopiedData(vec3f(0.f)));
    sphere.setParam("radius", 1.f);
    sphere.commit();

    cpp::GeometricModel model(sphere);
    cpp::Material material(rendererType, "obj");
    material.commit();
    model.setParam("material", material);
    model.commit();

    group.setParam("geometry", cpp::CopiedData(model));
    group.commit();
  }
  AddInstance(cpp::Instance(group));

  // position camera
  camera.setParam("position", vec3f(0.f, 0.f, -3.f));

  // prepare environment texture
  cpp::Texture envTex("texture2d");
  {
    std::array<vec3f, 8> data = {vec3f(0.f, 1.f, 1.f),
        vec3f(1.f, 0.f, 1.f),
        vec3f(1.f, 1.f, 0.f),
        vec3f(1.f, 1.f, 1.f),
        vec3f(1.f, 0.f, 0.f),
        vec3f(0.f, 1.f, 0.f),
        vec3f(0.f, 0.f, 1.f),
        vec3f(0.f, 0.f, 0.f)};
    cpp::CopiedData texData(data.data(), vec2ul(4, 2));
    envTex.setParam("format", OSP_TEXTURE_RGB32F);
    envTex.setParam("filter", OSP_TEXTURE_FILTER_NEAREST);
    envTex.setParam("data", texData);
    envTex.commit();
  }

  // prepare light
  cpp::Light light("hdri");
  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("up", vec3f(0.f, 1.f, 0.f));
  light.setParam("direction", vec3f(0.f, 0.f, 1.f));
  light.setParam("map", envTex);
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
        ::testing::Values(0.0f),
        ::testing::Values(0.01f)));

INSTANTIATE_TEST_SUITE_P(Light2,
    SunSky,
    ::testing::Combine(::testing::Values(vec3f(0.2f, -0.5f, 0.f)),
        ::testing::Values(vec3f(0.2f, 0.4f, -1.f), vec3f(0.f, 0.f, -1.f)),
        ::testing::Values(2.0f),
        ::testing::Values(0.0f, 1.0f),
        ::testing::Values(0.1f)));

TEST_P(AmbientLight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(
    Light, AmbientLight, ::testing::Values("scivis", "pathtracer"));

TEST_P(DistantLight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Light,
    DistantLight,
    ::testing::Combine(
        ::testing::Values(vec3f(0.0f, 0.0f, 1.0f), vec3f(-0.5f, 1.0f, 3.0f)),
        ::testing::Values("scivis", "pathtracer")));

TEST_P(GeometricLight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Light,
    GeometricLight,
    ::testing::Combine(::testing::Values(0.2f, 0.4f), ::testing::Bool()));

TEST_P(PhotometricLight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Light,
    PhotometricLight,
    ::testing::Combine(::testing::Values(0.0f, 0.1f),
        ::testing::Values("scivis", "pathtracer")));

TEST_P(QuadLight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Light,
    QuadLight,
    ::testing::Combine(::testing::Values(0.2f, 0.4f),
        ::testing::Values("scivis", "pathtracer"),
        ::testing::Values(OSP_INTENSITY_QUANTITY_INTENSITY)));

TEST_P(SphereLight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Light,
    SphereLight,
    ::testing::Combine(::testing::Values(0.0f, 0.2f, 0.3f),
        ::testing::Values("scivis", "pathtracer"),
        ::testing::Values(OSP_INTENSITY_QUANTITY_INTENSITY)));

TEST_P(SpotLight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Light,
    SpotLight,
    ::testing::Combine(::testing::Values(vec2f(0.0f, 0.0f),
                           vec2f(0.0f, 0.2f),
                           vec2f(0.0f, 0.4f),
                           vec2f(0.2f, 0.4f),
                           vec2f(0.7f, 0.8f)),
        ::testing::Values("scivis", "pathtracer"),
        ::testing::Values(OSP_INTENSITY_QUANTITY_INTENSITY)));

INSTANTIATE_TEST_SUITE_P(LightIntensityQuantity,
    QuadLight,
    ::testing::Combine(::testing::Values(0.2f, 0.4f),
        ::testing::Values("pathtracer"),
        ::testing::Values(
            OSP_INTENSITY_QUANTITY_RADIANCE, OSP_INTENSITY_QUANTITY_POWER)));

INSTANTIATE_TEST_SUITE_P(LightIntensityQuantity,
    SphereLight,
    ::testing::Combine(::testing::Values(0.0f, 0.2f, 0.3f),
        ::testing::Values("pathtracer"),
        ::testing::Values(
            OSP_INTENSITY_QUANTITY_RADIANCE, OSP_INTENSITY_QUANTITY_POWER)));

INSTANTIATE_TEST_SUITE_P(LightIntensityQuantity,
    SpotLight,
    ::testing::Combine(
        ::testing::Values(
            vec2f(0.0f, 0.0f), vec2f(0.0f, 0.2f), vec2f(0.0f, 0.4f)),
        ::testing::Values("pathtracer"),
        ::testing::Values(
            OSP_INTENSITY_QUANTITY_RADIANCE, OSP_INTENSITY_QUANTITY_POWER)));

TEST_P(HDRILight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(
    Light, HDRILight, ::testing::Values("scivis", "pathtracer"));

} // namespace OSPRayTestScenes
