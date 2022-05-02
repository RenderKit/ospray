// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_light.h"
#include "ospray_testing.h"

namespace OSPRayTestScenes {

LightTest::LightTest()
{
  samplesPerPixel = 16;

  // due to the way c0 in SpotLight is calculated we must rotate around X axis
  // to get the same result with SciVis and ring light (because its
  // approximation in SciVis is not rotation invariant)
  xfm = affine3f::translate(vec3f(1.f, 2.f, 3.f))
      * affine3f::rotate(vec3f(1.f, 0.f, 0.f), pi);
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

void LightTest::AddInstancedLightWithMB(
    cpp::Light light, const affine3f &xfm1, const affine3f &xfm2)
{
  light.commit();
  cpp::Group group;
  group.setParam("light", cpp::CopiedData(light));
  group.commit();
  cpp::Instance instance(group);
  const affine3f xfmR = rcp(xfm);
  if (motionBlur) {
    std::vector<affine3f> xfms;
    xfms.push_back(xfmR * xfm1);
    xfms.push_back(xfmR * xfm2);
    instance.setParam("motion.transform", cpp::CopiedData(xfms));
    camera.setParam("shutter", range1f(0, 1));
  } else
    instance.setParam("transform", xfmR);

  AddInstance(instance);
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
  motionBlur = std::get<2>(params);
}

void DistantLight::SetUp()
{
  LightTest::SetUp();

  cpp::Light distant("distant");
  distant.setParam("direction", xfmVector(xfm, direction));
  distant.setParam("color", vec3f(1.0f, 0.75f, 0.25f));
  distant.setParam("angularDiameter", 1.0f);

  AddInstancedLightWithRotateMB(distant);
}

GeometricLight::GeometricLight()
{
  rendererType = "pathtracer";
  auto params = GetParam();
  size = std::get<0>(params);
  useMaterialList = std::get<1>(params);
  motionBlur = std::get<2>(params);
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
  lightMesh.setParam("index", cpp::CopiedData(lightIndices));
  if (motionBlur == 2) { // deformation
    for (vec3f &p : lightVertices)
      p.x -= 0.5f;
    std::vector<cpp::CopiedData> mposdata;
    mposdata.push_back(cpp::CopiedData(lightVertices));
    for (vec3f &p : lightVertices)
      p.x += 1.0f;
    mposdata.push_back(cpp::CopiedData(lightVertices));
    lightMesh.setParam("motion.vertex.position", cpp::CopiedData(mposdata));
  } else
    lightMesh.setParam("vertex.position", cpp::CopiedData(lightVertices));
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

  if (motionBlur == 1) { // instance
    lightModel.commit();
    cpp::Group group;
    group.setParam("geometry", cpp::CopiedData(lightModel));
    group.commit();
    cpp::Instance instance(group);
    std::vector<affine3f> xfms;
    xfms.push_back(affine3f::translate(vec3f(-0.5, 0, 0)));
    xfms.push_back(affine3f::translate(vec3f(0.5, 0, 0)));
    instance.setParam("motion.transform", cpp::CopiedData(xfms));
    AddInstance(instance);
  } else
    AddModel(lightModel);

  if (motionBlur)
    camera.setParam("shutter", range1f(0, 1));
}

PhotometricLight::PhotometricLight()
{
  auto params = GetParam();
  lightType = std::get<0>(params);
  size = std::get<1>(params);
  rendererType = std::get<2>(params);

  // area lights need a minimum size
  if (lightType == "quad")
    size = std::max(0.01f, size);
}

void PhotometricLight::SetUp()
{
  LightTest::SetUp();

  const vec3f pos1 = xfmPoint(xfm, vec3f(-0.6f, 0.8f, -0.5f));
  const vec3f pos2 = xfmPoint(xfm, vec3f(0.3f, 0.6f, 0.f));
  const vec3f dir = xfmVector(xfm, vec3f(0.0f, -1.0f, 0.0f));
  const vec3f edge1 = xfmVector(xfm, vec3f(1.0f, 0.0f, 0.0f));
  const vec3f edge2 = xfmVector(xfm, vec3f(0.0f, 0.0f, 1.0f));

  cpp::Light light1d(lightType);
  light1d.setParam("intensity", 5.f);
  light1d.setParam("intensityQuantity", OSP_INTENSITY_QUANTITY_SCALE);
  float lid1d[] = {2.5f, 0.4f, 0.2f, 0.1f, 0.03f, 0.01f, 0.01f};
  light1d.setParam("intensityDistribution", cpp::CopiedData(lid1d, 7));

  if (lightType == "spot") {
    light1d.setParam("position", pos1);
    light1d.setParam("direction", dir);
    light1d.setParam("openingAngle", 360.f);
    light1d.setParam("penumbraAngle", 0.f);
    light1d.setParam("radius", size);
  } else if (lightType == "quad") {
    light1d.setParam("position", pos1 - size * edge1 - size * edge2);
    light1d.setParam("edge1", 2.0f * size * edge1);
    light1d.setParam("edge2", 2.0f * size * edge2);
  } else if (lightType == "sphere") {
    light1d.setParam("position", pos1);
    light1d.setParam("radius", size);
    light1d.setParam("direction", dir);
  }

  light1d.commit();

  cpp::Light light2d(lightType);
  light2d.setParam("intensity", 1.f);
  light2d.setParam("intensityQuantity", OSP_INTENSITY_QUANTITY_SCALE);
  float lid2d[60] = {
      1.5f, 5.0f, 6.0f, 0.3f, 0.01f, 0.15f, 0.5f, 1.6f, 0.1f, 0.01f};
  light2d.setParam(
      "intensityDistribution", cpp::CopiedData(lid2d, vec2ul(5, 12)));
  light2d.setParam("c0", xfmVector(xfm, vec3f(1.0f, 0.0f, 0.0f)));

  if (lightType == "spot") {
    light2d.setParam("position", pos2);
    light2d.setParam("direction", dir);
    light2d.setParam("openingAngle", 270.f);
    light2d.setParam("penumbraAngle", 10.f);
    light2d.setParam("radius", size);
  } else if (lightType == "quad") {
    light2d.setParam("position", pos2 - size * edge1 - size * edge2);
    light2d.setParam("edge1", 2.0f * size * edge1);
    light2d.setParam("edge2", 2.0f * size * edge2);
  } else if (lightType == "sphere") {
    light2d.setParam("position", pos2);
    light2d.setParam("radius", size);
    light2d.setParam("direction", dir);
  }

  light2d.commit();

  cpp::Group group;
  std::vector<cpp::Light> lights{light1d, light2d};
  group.setParam("light", cpp::CopiedData(lights));
  group.commit();
  cpp::Instance instance(group);
  instance.setParam("transform", rcp(xfm));
  AddInstance(instance);
}

QuadLight::QuadLight()
{
  auto params = GetParam();
  size = std::get<0>(params);
  rendererType = std::get<1>(params);
  intensityQuantity = std::get<2>(params);
  motionBlur = std::get<3>(params);
}

void QuadLight::SetUp()
{
  LightTest::SetUp();

  cpp::Light light("quad");
  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 10.f);
  light.setParam("intensityQuantity", intensityQuantity);
  light.setParam(
      "position", xfmPoint(xfm, vec3f(size / -2.0f, 0.98f, size / -2.0f)));
  light.setParam("edge1", xfmVector(xfm, vec3f(size, 0.0f, 0.0f)));
  light.setParam("edge2", xfmVector(xfm, vec3f(0.0f, 0.0f, size)));

  AddInstancedLightWithTranslateMB(light);
}

CylinderLight::CylinderLight()
{
  auto params = GetParam();
  size = std::get<0>(params);
  rendererType = std::get<1>(params);
  intensityQuantity = std::get<2>(params);
  motionBlur = std::get<3>(params);
}

void CylinderLight::SetUp()
{
  LightTest::SetUp();

  cpp::Light light("cylinder");
  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 5.0f);
  light.setParam("intensityQuantity", intensityQuantity);
  light.setParam(
      "position0", xfmPoint(xfm, vec3f(-0.2f - 2.0 * size, 0.65f, 0.0f)));
  light.setParam(
      "position1", xfmPoint(xfm, vec3f(0.2f + 2.0 * size, 0.65f, 0.0f)));
  light.setParam("radius", size);

  AddInstancedLightWithTranslateMB(light);
}

SphereLight::SphereLight()
{
  auto params = GetParam();
  radius = std::get<0>(params);
  rendererType = std::get<1>(params);
  intensityQuantity = std::get<2>(params);
  motionBlur = std::get<3>(params);
}

void SphereLight::SetUp()
{
  LightTest::SetUp();

  cpp::Light light("sphere");
  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 2.5f);
  light.setParam("intensityQuantity", intensityQuantity);
  light.setParam("position", xfmPoint(xfm, vec3f(0.0f, 0.48f, 0.0f)));
  light.setParam("radius", radius);

  AddInstancedLightWithTranslateMB(light);
}

SpotLight::SpotLight()
{
  auto params = GetParam();
  innerOuterRadius = std::get<0>(params);
  rendererType = std::get<1>(params);
  intensityQuantity = std::get<2>(params);
  motionBlur = std::get<3>(params);
}

void SpotLight::SetUp()
{
  LightTest::SetUp();

  cpp::Light light("spot");
  light.setParam("color", vec3f(0.78f, 0.551f, 0.183f));
  light.setParam("intensity", 10.f);
  light.setParam("intensityQuantity", intensityQuantity);
  light.setParam("position", xfmPoint(xfm, vec3f(0.0f, 0.98f, 0.0f)));
  light.setParam("direction", xfmVector(xfm, vec3f(0.0f, -1.0f, 0.0f)));
  light.setParam("radius", innerOuterRadius[1]);
  light.setParam("innerRadius", innerOuterRadius[0]);

  AddInstancedLightWithTranslateMB(light);
}

HDRILight::HDRILight()
{
  auto params = GetParam();
  rendererType = std::get<0>(params);
  motionBlur = std::get<1>(params);
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
  light.setParam("up", xfmVector(xfm, vec3f(0.f, 1.f, 0.f)));
  light.setParam("direction", xfmVector(xfm, vec3f(0.f, 0.f, 1.f)));
  light.setParam("map", envTex);

  AddInstancedLightWithRotateMB(light);

  renderer.setParam("backgroundColor", vec4f(0.f, 0.f, 0.f, 1.0f));
}

SunSky::SunSky()
{
  rendererType = "pathtracer";
  samplesPerPixel = 1;
  auto params = GetParam();
  motionBlur = std::get<5>(params);
}

void SunSky::SetUp()
{
  Base::SetUp();
  auto params = GetParam();

  cpp::Light light("sunSky");
  float turb = std::get<2>(params);
  light.setParam("up", xfmVector(xfm, std::get<0>(params)));
  light.setParam("direction", xfmVector(xfm, std::get<1>(params)));
  light.setParam("turbidity", turb);
  light.setParam("albedo", std::get<3>(params));
  // lower brightness with high turbidity
  light.setParam("intensityQuantity", OSP_INTENSITY_QUANTITY_SCALE);
  light.setParam("intensity", 0.025f / turb);
  light.setParam("horizonExtension", std::get<4>(params));

  AddInstancedLightWithRotateMB(light);

  renderer.setParam("backgroundColor", vec4f(0.f, 0.f, 0.f, 1.0f));
}

// Test Instantiations //////////////////////////////////////////////////////

// Ambient Light

TEST_P(AmbientLight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(
    Light, AmbientLight, ::testing::Values("scivis", "pathtracer"));

// Distant Light

TEST_P(DistantLight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Light,
    DistantLight,
    ::testing::Combine(
        ::testing::Values(vec3f(0.0f, 0.0f, 1.0f), vec3f(-0.5f, 1.0f, 3.0f)),
        ::testing::Values("scivis", "pathtracer"),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(LightMotionBlur,
    DistantLight,
    ::testing::Values(
        std::make_tuple(vec3f(-0.5f, 1.0f, 3.0f), "pathtracer", true)));

// Geometric Light

TEST_P(GeometricLight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Light,
    GeometricLight,
    ::testing::Combine(::testing::Values(0.2f, 0.4f),
        ::testing::Bool(),
        ::testing::Values(0)));

INSTANTIATE_TEST_SUITE_P(LightMotionBlur,
    GeometricLight,
    ::testing::Combine(::testing::Values(0.2f),
        ::testing::Values(false),
        ::testing::Values(1, 2)));

// Quad Light

TEST_P(QuadLight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Light,
    QuadLight,
    ::testing::Combine(::testing::Values(0.2f, 0.4f),
        ::testing::Values("scivis", "pathtracer"),
        ::testing::Values(OSP_INTENSITY_QUANTITY_INTENSITY),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(LightIntensityQuantity,
    QuadLight,
    ::testing::Combine(::testing::Values(0.2f, 0.4f),
        ::testing::Values("pathtracer"),
        ::testing::Values(
            OSP_INTENSITY_QUANTITY_RADIANCE, OSP_INTENSITY_QUANTITY_POWER),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(LightMotionBlur,
    QuadLight,
    ::testing::Values(std::make_tuple(
        0.2f, "pathtracer", OSP_INTENSITY_QUANTITY_INTENSITY, true)));

// Cylinder Light

TEST_P(CylinderLight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Light,
    CylinderLight,
    ::testing::Combine(::testing::Values(0.02f, 0.15f),
        ::testing::Values("scivis", "pathtracer"),
        ::testing::Values(OSP_INTENSITY_QUANTITY_INTENSITY),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(LightIntensityQuantity,
    CylinderLight,
    ::testing::Combine(::testing::Values(0.02f, 0.15),
        ::testing::Values("pathtracer"),
        ::testing::Values(
            OSP_INTENSITY_QUANTITY_RADIANCE, OSP_INTENSITY_QUANTITY_POWER),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(LightMotionBlur,
    CylinderLight,
    ::testing::Values(std::make_tuple(
        0.02f, "pathtracer", OSP_INTENSITY_QUANTITY_INTENSITY, true)));

// Sphere Light

TEST_P(SphereLight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Light,
    SphereLight,
    ::testing::Combine(::testing::Values(0.0f, 0.2f, 0.3f),
        ::testing::Values("scivis", "pathtracer"),
        ::testing::Values(OSP_INTENSITY_QUANTITY_INTENSITY),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(LightIntensityQuantity,
    SphereLight,
    ::testing::Combine(::testing::Values(0.0f, 0.2f, 0.3f),
        ::testing::Values("pathtracer"),
        ::testing::Values(
            OSP_INTENSITY_QUANTITY_RADIANCE, OSP_INTENSITY_QUANTITY_POWER),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(LightMotionBlur,
    SphereLight,
    ::testing::Values(std::make_tuple(
        0.3f, "pathtracer", OSP_INTENSITY_QUANTITY_RADIANCE, true)));

// Spot Light

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
        ::testing::Values(OSP_INTENSITY_QUANTITY_INTENSITY),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(LightIntensityQuantity,
    SpotLight,
    ::testing::Combine(
        ::testing::Values(
            vec2f(0.0f, 0.0f), vec2f(0.0f, 0.2f), vec2f(0.0f, 0.4f)),
        ::testing::Values("pathtracer"),
        ::testing::Values(
            OSP_INTENSITY_QUANTITY_RADIANCE, OSP_INTENSITY_QUANTITY_POWER),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(LightMotionBlur,
    SpotLight,
    ::testing::Values(std::make_tuple(vec2f(0.0f, 0.4f),
        "pathtracer",
        OSP_INTENSITY_QUANTITY_RADIANCE,
        true)));

// Photometric (Spot) Light

TEST_P(PhotometricLight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Light,
    PhotometricLight,
    ::testing::Combine(::testing::Values("spot", "quad", "sphere"),
        ::testing::Values(0.0f, 0.1f),
        ::testing::Values("scivis", "pathtracer")));

// HDRI Light

TEST_P(HDRILight, parameter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Light,
    HDRILight,
    ::testing::Combine(
        ::testing::Values("scivis", "pathtracer"), ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(LightMotionBlur,
    HDRILight,
    ::testing::Values(std::make_tuple("pathtracer", true)));

// SunSky Light

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
        ::testing::Values(0.01f),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(Light2,
    SunSky,
    ::testing::Combine(::testing::Values(vec3f(0.2f, -0.5f, 0.f)),
        ::testing::Values(vec3f(0.2f, 0.4f, -1.f), vec3f(0.f, 0.f, -1.f)),
        ::testing::Values(2.0f),
        ::testing::Values(0.0f, 1.0f),
        ::testing::Values(0.1f),
        ::testing::Values(false)));

INSTANTIATE_TEST_SUITE_P(LightMotionBlur,
    SunSky,
    ::testing::Values(std::make_tuple(vec3f(0.2f, -0.5f, 0.f),
        vec3f(0.2f, 0.4f, -1.f),
        2.0f,
        0.0f,
        0.1f,
        true)));

} // namespace OSPRayTestScenes
