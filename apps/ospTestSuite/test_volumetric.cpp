// Copyright 2017 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_volumetric.h"
#include "ArcballCamera.h"
#include "ospray_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"

namespace OSPRayTestScenes {

// Helper functions /////////////////////////////////////////////////////////

// creates a torus
// volumetric data: stores data of torus
// returns created ospvolume of torus
static cpp::Volume CreateTorus(const int size)
{
  std::vector<float> volumetricData(size * size * size);

  const float r = 30;
  const float R = 80;
  const int size_2 = size / 2;
  const vec3i dims = vec3i(size);
  rkcommon::index_sequence_3D idx(dims);
  for (vec3i i : idx) {
    const float X = i.x - size_2;
    const float Y = i.y - size_2;
    const float Z = i.z - size_2;

    const float d = (R - std::sqrt(Z * Z + Y * Y));
    volumetricData[idx.flatten(i)] = r * r - d * d - X * X;
  }

  cpp::Volume torus("structuredRegular");
  torus.setParam("data", cpp::CopiedData(volumetricData.data(), vec3ul(size)));
  torus.setParam("gridOrigin", vec3f(-0.5f));
  torus.setParam("gridSpacing", vec3f(1.f / size));
  torus.commit();
  return torus;
}

/////////////////////////////////////////////////////////////////////////////

TextureVolumeTransform_deprecated::TextureVolumeTransform_deprecated()
{
  rendererType = GetParam();
}

void TextureVolumeTransform_deprecated::SetUp()
{
  Base::SetUp();

  camera.setParam("position", vec3f(.66f, .66f, -2.5f));
  camera.setParam("direction", vec3f(0.f, 0.f, 1.f));
  camera.setParam("up", vec3f(0.f, 1.f, 0.f));

  // Create transfer function
  cpp::TransferFunction transferFun("piecewiseLinear");
  {
    std::vector<vec3f> colors = {vec3f(1.f, 0.f, 0.f),
        vec3f(0.f, 1.f, 0.f),
        vec3f(0.f, 1.f, 1.f),
        vec3f(1.f, 1.f, 0.f),
        vec3f(1.f, 1.f, 1.f),
        vec3f(1.f, 0.f, 1.f)};
    std::vector<float> opacities = {1.f, 1.f};

    transferFun.setParam("valueRange", vec2f(-10000.f, 100.f));
    transferFun.setParam("color", cpp::CopiedData(colors));
    transferFun.setParam("opacity", cpp::CopiedData(opacities));
    transferFun.commit();
  }

  // Create volumetric model
  cpp::VolumetricModel volumetricModel(CreateTorus(256));
  volumetricModel.setParam("transferFunction", transferFun);
  volumetricModel.commit();

  // Create volume texture
  cpp::Texture tex("volume");
  tex.setParam("volume", volumetricModel);
  tex.commit();

  // Create a single sphere geometry
  cpp::Geometry sphere("sphere");
  {
    sphere.setParam("sphere.position", cpp::CopiedData(vec3f(0.f)));
    sphere.setParam("radius", 0.51f);
    sphere.commit();
  }

  // Prepare material array
  constexpr uint32_t cols = 2;
  constexpr uint32_t rows = 2;
  std::array<cpp::Material, cols * rows> materials;
  {
    // Create materials
    for (uint32_t i = 0; i < cols * rows; i++) {
      cpp::Material mat(rendererType, "obj");
      mat.setParam("map_kd", tex);
      mat.commit();
      materials[i] = mat;
    }

    // Set scale
    materials[1].setParam("map_kd.transform", affine3f::scale(vec3f(1.2f)));
    materials[1].commit();

    // Set rotation
    materials[2].setParam(
        "map_kd.transform", affine3f::rotate(vec3f(0.5, 0.2, 1), 1.f));
    materials[2].commit();
  }

  // Prepare instances
  rkcommon::index_sequence_2D idx(vec2i(cols, rows));
  for (auto i : idx) {
    // Create geometric model
    cpp::GeometricModel model(sphere);
    model.setParam("material", materials[idx.flatten(i)]);
    model.commit();

    // Create group
    cpp::Group group;
    group.setParam("geometry", cpp::CopiedData(model));
    group.commit();

    // Create instance
    cpp::Instance instance(group);
    instance.setParam(
        "transform", affine3f::translate(1.25f * vec3f(i.x, i.y, 0.f)));
    instance.commit();
    AddInstance(instance);
  }

  cpp::Light ambient("ambient");
  ambient.setParam("intensity", 0.5f);
  AddLight(ambient);
}

TextureVolumeTransform::TextureVolumeTransform()
{
  rendererType = GetParam();
}

void TextureVolumeTransform::SetUp()
{
  Base::SetUp();

  camera.setParam("position", vec3f(.66f, .66f, -2.5f));
  camera.setParam("direction", vec3f(0.f, 0.f, 1.f));
  camera.setParam("up", vec3f(0.f, 1.f, 0.f));

  // Create transfer function
  cpp::TransferFunction transferFun("piecewiseLinear");
  {
    std::vector<vec3f> colors = {vec3f(1.f, 0.f, 0.f),
        vec3f(0.f, 1.f, 0.f),
        vec3f(0.f, 1.f, 1.f),
        vec3f(1.f, 1.f, 0.f),
        vec3f(1.f, 1.f, 1.f),
        vec3f(1.f, 0.f, 1.f)};
    std::vector<float> opacities = {1.f, 1.f};

    transferFun.setParam("valueRange", vec2f(-10000.f, 100.f));
    transferFun.setParam("color", cpp::CopiedData(colors));
    transferFun.setParam("opacity", cpp::CopiedData(opacities));
    transferFun.commit();
  }

  // Create volumetric model
  cpp::Volume volume = CreateTorus(256);
  volume.commit();

  // Create volume texture
  cpp::Texture tex("volume");
  tex.setParam("volume", volume);
  tex.setParam("transferFunction", transferFun);
  tex.commit();

  // Create a single sphere geometry
  cpp::Geometry sphere("sphere");
  {
    sphere.setParam("sphere.position", cpp::CopiedData(vec3f(0.f)));
    sphere.setParam("radius", 0.51f);
    sphere.commit();
  }

  // Prepare material array
  constexpr uint32_t cols = 2;
  constexpr uint32_t rows = 2;
  std::array<cpp::Material, cols * rows> materials;
  {
    // Create materials
    for (uint32_t i = 0; i < cols * rows; i++) {
      cpp::Material mat(rendererType, "obj");
      mat.setParam("map_kd", tex);
      mat.commit();
      materials[i] = mat;
    }

    // Set scale
    materials[1].setParam("map_kd.transform", affine3f::scale(vec3f(1.2f)));
    materials[1].commit();

    // Set rotation
    materials[2].setParam(
        "map_kd.transform", affine3f::rotate(vec3f(0.5, 0.2, 1), 1.f));
    materials[2].commit();
  }

  // Prepare instances
  rkcommon::index_sequence_2D idx(vec2i(cols, rows));
  for (auto i : idx) {
    // Create geometric model
    cpp::GeometricModel model(sphere);
    model.setParam("material", materials[idx.flatten(i)]);
    model.commit();

    // Create group
    cpp::Group group;
    group.setParam("geometry", cpp::CopiedData(model));
    group.commit();

    // Create instance
    cpp::Instance instance(group);
    instance.setParam(
        "transform", affine3f::translate(1.25f * vec3f(i.x, i.y, 0.f)));
    instance.commit();
    AddInstance(instance);
  }

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
  cpp::VolumetricModel volumetricModel(torus);

  cpp::TransferFunction transferFun("piecewiseLinear");
  transferFun.setParam("valueRange", vec2f(-10000.f, 10000.f));

  std::vector<vec3f> colors = {
      vec3f(1.0f, 0.0f, 0.0f), vec3f(0.0f, 1.0f, 0.0f)};

  std::vector<float> opacities = {0.05f, 1.0f};

  transferFun.setParam("color", cpp::CopiedData(colors));
  transferFun.setParam("opacity", cpp::CopiedData(opacities));
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
  depthTex.setParam("data", cpp::CopiedData(texData.data(), imgSize));
  depthTex.commit();

  renderer.setParam("map_maxDepth", depthTex);
  renderer.setParam("backgroundColor", bgColor);

  cpp::Light ambient("ambient");
  ambient.setParam("intensity", 0.5f);
  AddLight(ambient);
}

UnstructuredVolume::UnstructuredVolume()
{
  auto params = GetParam();
  rendererType = std::get<0>(params);
  showCells = std::get<1>(params);
}

void UnstructuredVolume::SetUp()
{
  Base::SetUp();

  instances.clear();

  auto builder = ospray::testing::newBuilder("unstructured_volume");
  ospray::testing::setParam(builder, "rendererType", rendererType);
  ospray::testing::setParam(builder, "showCells", showCells);
  ospray::testing::commit(builder);

  world = ospray::testing::buildWorld(builder);
  ospray::testing::release(builder);

  world.commit();

  auto worldBounds = world.getBounds<box3f>();

  ArcballCamera arcballCamera(worldBounds, imgSize);

  camera.setParam("position", arcballCamera.eyePos());
  camera.setParam("direction", arcballCamera.lookDir());
  camera.setParam("up", arcballCamera.upDir());

  renderer.setParam("maxPathLength", 1);
}

// Test Instantiations //////////////////////////////////////////////////////

INSTANTIATE_TEST_SUITE_P(TestScenesVolumes,
    FromOsprayTesting,
    ::testing::Combine(::testing::Values("gravity_spheres_volume",
                           "perlin_noise_volumes",
                           "unstructured_volume_simple",
                           "particle_volume",
                           "vdb_volume",
                           "vdb_volume_packed",
                           "gravity_spheres_amr"),
        ::testing::Values("scivis", "pathtracer", "ao"),
        ::testing::Values(16)));

INSTANTIATE_TEST_SUITE_P(TestScenesVolumesStrictParams,
    FromOsprayTesting,
    ::testing::Values(std::make_tuple("perlin_noise_many_volumes", "scivis", 4),
        std::make_tuple("perlin_noise_many_volumes", "pathtracer", 32),
        std::make_tuple("perlin_noise_many_volumes", "ao", 4)));

TEST_P(UnstructuredVolume, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(TestScenesVolumes,
    UnstructuredVolume,
    ::testing::Combine(::testing::Values("scivis", "pathtracer", "ao"),
        ::testing::Values(false, true)));

TEST_P(TextureVolumeTransform_deprecated, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(
    Renderers, TextureVolumeTransform_deprecated, ::testing::Values("scivis"));

TEST_P(TextureVolumeTransform, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(
    Renderers, TextureVolumeTransform, ::testing::Values("scivis"));

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
