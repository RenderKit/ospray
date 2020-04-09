// Copyright 2017-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_volumetric.h"

namespace OSPRayTestScenes {

// Helper functions /////////////////////////////////////////////////////////

// creates a torus
// volumetric data: stores data of torus
// returns created ospvolume of torus
static cpp::Volume CreateTorus(const int size)
{
  std::vector<float> volumetricData(size * size * size, 0);

  const float r = 30;
  const float R = 80;
  const int size_2 = size / 2;

  for (int x = 0; x < size; ++x) {
    for (int y = 0; y < size; ++y) {
      for (int z = 0; z < size; ++z) {
        const float X = x - size_2;
        const float Y = y - size_2;
        const float Z = z - size_2;

        const float d = (R - std::sqrt(X * X + Y * Y));
        volumetricData[size * size * x + size * y + z] = r * r - d * d - Z * Z;
      }
    }
  }

  cpp::Volume torus("structuredRegular");
  torus.setParam("data", cpp::Data(vec3ul(size), volumetricData.data()));
  torus.setParam("gridOrigin", vec3f(-0.5f, -0.5f, -0.5f));
  torus.setParam("gridSpacing", vec3f(1.f / size, 1.f / size, 1.f / size));
  return torus;
}

/////////////////////////////////////////////////////////////////////////////

TextureVolume::TextureVolume()
{
  rendererType = GetParam();
}

void TextureVolume::SetUp()
{
  Base::SetUp();

  camera.setParam("position", vec3f(-0.7f, -1.4f, 0.f));
  camera.setParam("direction", vec3f(0.5f, 1.f, 0.f));
  camera.setParam("up", vec3f(0.f, 0.f, -1.f));

  cpp::Volume torus = CreateTorus(256);
  torus.commit();

  cpp::VolumetricModel volumetricModel(torus);

  cpp::TransferFunction transferFun("piecewiseLinear");
  transferFun.setParam("valueRange", vec2f(-10000.f, 10000.f));

  std::vector<vec3f> colors = {
      vec3f(1.0f, 0.0f, 0.0f), vec3f(0.0f, 1.0f, 0.0f)};

  std::vector<float> opacities = {1.0f, 1.0f};

  transferFun.setParam("color", cpp::Data(colors));
  transferFun.setParam("opacity", cpp::Data(opacities));
  transferFun.commit();

  volumetricModel.setParam("transferFunction", transferFun);
  volumetricModel.commit();

  cpp::Texture tex("volume");
  tex.setParam("volume", volumetricModel);
  tex.commit();

  cpp::Material sphereMaterial(rendererType, "obj");
  sphereMaterial.setParam("map_kd", tex);
  sphereMaterial.commit();

  cpp::Geometry sphere("sphere");
  sphere.setParam("sphere.position", cpp::Data(vec3f(0.f)));
  sphere.setParam("radius", 0.51f);
  sphere.commit();

  cpp::GeometricModel model(sphere);
  model.setParam("material", sphereMaterial);
  AddModel(model);

  cpp::Light ambient("ambient");
  ambient.setParam("intensity", 0.5f);
  AddLight(ambient);
}

DepthCompositeVolume::DepthCompositeVolume()
{
  auto params = GetParam();
  rendererType = std::get<0>(params);
  bgColor = std::get<1>(params);
}

void DepthCompositeVolume::SetUp()
{
  Base::SetUp();

  camera.setParam("position", vec3f(0.f, 0.f, 1.f));
  camera.setParam("direction", vec3f(0.f, 0.f, -1.f));
  camera.setParam("up", vec3f(0.f, 1.f, 0.f));

  cpp::Volume torus = CreateTorus(256);
  torus.commit();

  cpp::VolumetricModel volumetricModel(torus);

  cpp::TransferFunction transferFun("piecewiseLinear");
  transferFun.setParam("valueRange", vec2f(-10000.f, 10000.f));

  std::vector<vec3f> colors = {
      vec3f(1.0f, 0.0f, 0.0f), vec3f(0.0f, 1.0f, 0.0f)};

  std::vector<float> opacities = {0.05f, 1.0f};

  transferFun.setParam("color", cpp::Data(colors));
  transferFun.setParam("opacity", cpp::Data(opacities));
  transferFun.commit();

  volumetricModel.setParam("transferFunction", transferFun);
  volumetricModel.commit();

  AddModel(volumetricModel);

  cpp::Texture depthTex("texture2d");

  std::vector<float> texData(imgSize.product());
  for (int y = 0; y < imgSize.y; y++) {
    for (int x = 0; x < imgSize.x; x++) {
      const size_t index = imgSize.x * y + x;
      if (x < imgSize.x / 3) {
        texData[index] = 999.f;
      } else if (x < (imgSize.x * 2) / 3) {
        texData[index] = 0.75f;
      } else {
        texData[index] = 0.00001f;
      }
    }
  }

  depthTex.setParam("format", OSP_TEXTURE_R32F);
  depthTex.setParam("filter", OSP_TEXTURE_FILTER_NEAREST);
  depthTex.setParam("data", cpp::Data(imgSize, texData.data()));
  depthTex.commit();

  renderer.setParam("map_maxDepth", depthTex);
  renderer.setParam("backgroundColor", bgColor);

  cpp::Light ambient("ambient");
  ambient.setParam("intensity", 0.5f);
  AddLight(ambient);
}

// Test Instantiations //////////////////////////////////////////////////////

INSTANTIATE_TEST_SUITE_P(TestScenesVolumes,
    FromOsprayTesting,
    ::testing::Combine(::testing::Values("gravity_spheres_volume",
                           "perlin_noise_volumes",
                           "unstructured_volume"),
        ::testing::Values("scivis", "pathtracer")));

TEST_P(TextureVolume, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Renderers, TextureVolume, ::testing::Values("scivis"));

TEST_P(DepthCompositeVolume, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Renderers,
    DepthCompositeVolume,
    ::testing::Combine(::testing::Values("scivis"),
        ::testing::Values(vec4f(0.f),
            vec4f(1.f),
            vec4f(0.f, 0.f, 0.f, 1.f),
            vec4f(1.f, 0.f, 0.f, 0.5f))));

} // namespace OSPRayTestScenes
