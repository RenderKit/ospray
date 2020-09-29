// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Builder.h"
#include "Noise.h"
#include "ospray_testing.h"
#include "rkcommon/tasking/parallel_for.h"
// stl
#include <functional>

using namespace rkcommon::math;

namespace ospray {
namespace testing {

struct PerlinNoiseVolumes : public detail::Builder
{
  PerlinNoiseVolumes(bool clip = false, bool gradientShading = false);
  ~PerlinNoiseVolumes() override = default;

  void commit() override;

  cpp::Group buildGroup() const override;
  cpp::World buildWorld() const override;

 private:
  bool addSphereVolume{true};
  bool addTorusVolume{false};
  bool addBoxes{false};

  bool addAreaLight{true};
  bool addAmbientLight{false};

  float densityScale{10.f};
  float anisotropy{0.f};
  float gradientShadingScale{0.f};

  bool withClipping{false};
};

// Inlined definitions ////////////////////////////////////////////////////

// Helper functions //

cpp::Geometry makeBoxGeometry(const box3f &box)
{
  cpp::Geometry ospGeometry("box");
  ospGeometry.setParam("box", cpp::CopiedData(box));
  ospGeometry.commit();
  return ospGeometry;
}

cpp::VolumetricModel createProceduralVolumetricModel(
    std::function<bool(vec3f p)> D,
    std::vector<vec3f> colors,
    std::vector<float> opacities,
    float densityScale,
    float anisotropy,
    float gradientShadingScale)
{
  vec3ul dims{128}; // should be at least 2
  const float spacing = 3.f / (reduce_max(dims) - 1);
  cpp::Volume volume("structuredRegular");

  // generate volume data
  auto numVoxels = dims.product();
  std::vector<float> voxels(numVoxels, 0);
  tasking::parallel_for(dims.z, [&](uint64_t z) {
    for (uint64_t y = 0; y < dims.y; ++y) {
      for (uint64_t x = 0; x < dims.x; ++x) {
        vec3f p = vec3f(x + 0.5f, y + 0.5f, z + 0.5f) / dims;
        if (D(p))
          voxels[dims.x * dims.y * z + dims.x * y + x] =
              0.5f + 0.5f * PerlinNoise::noise(p, 12);
      }
    }
  });

  // calculate voxel range
  range1f voxelRange;
  std::for_each(voxels.begin(), voxels.end(), [&](float &v) {
    if (!std::isnan(v))
      voxelRange.extend(v);
  });

  volume.setParam("data", cpp::CopiedData(voxels.data(), dims));
  volume.setParam("gridOrigin", vec3f(-1.5f, -1.5f, -1.5f));
  volume.setParam("gridSpacing", vec3f(spacing));
  volume.commit();

  cpp::TransferFunction tfn("piecewiseLinear");
  tfn.setParam("valueRange", voxelRange.toVec2());
  tfn.setParam("color", cpp::CopiedData(colors));
  tfn.setParam("opacity", cpp::CopiedData(opacities));
  tfn.commit();

  cpp::VolumetricModel volumeModel(volume);
  volumeModel.setParam("densityScale", densityScale);
  volumeModel.setParam("anisotropy", anisotropy);
  volumeModel.setParam("transferFunction", tfn);
  volumeModel.setParam("gradientShadingScale", gradientShadingScale);
  volumeModel.commit();

  return volumeModel;
}

cpp::GeometricModel createGeometricModel(
    cpp::Geometry geo, const std::string &rendererType, const vec3f &kd)
{
  cpp::GeometricModel geometricModel(geo);

  if (rendererType == "pathtracer" || rendererType == "scivis"
      || rendererType == "ao") {
    cpp::Material objMaterial(rendererType, "obj");
    objMaterial.setParam("kd", kd);
    objMaterial.commit();
    geometricModel.setParam("material", objMaterial);
  }

  return geometricModel;
}

// PerlineNoiseVolumes definitions //

PerlinNoiseVolumes::PerlinNoiseVolumes(bool clip, bool gradientShading)
    : gradientShadingScale(gradientShading), withClipping(clip)
{}

void PerlinNoiseVolumes::commit()
{
  Builder::commit();

  addSphereVolume = getParam<bool>("addSphereVolume", true);
  addTorusVolume = getParam<bool>("addTorusVolume", true);
  addBoxes = getParam<bool>("addBoxes", false);

  addAreaLight = getParam<bool>("addAreaLight", true);
  addAmbientLight = getParam<bool>("addAmbientLight", true);

  densityScale = getParam<float>("densityScale", 10.f);
  anisotropy = getParam<float>("anisotropy", 0.f);
  gradientShadingScale =
      getParam<float>("gradientShadingScale", gradientShadingScale);

  withClipping = getParam<bool>("withClipping", withClipping);
}

cpp::Group PerlinNoiseVolumes::buildGroup() const
{
  cpp::Group group;
  std::vector<cpp::VolumetricModel> volumetricModels;

  if (addSphereVolume) {
    volumetricModels.emplace_back(createProceduralVolumetricModel(
        [](const vec3f &p) { return turbulentSphere(p, 1.f); },
        {vec3f(0.f, 0.0f, 0.f),
            vec3f(1.f, 0.f, 0.f),
            vec3f(0.f, 1.f, 1.f),
            vec3f(1.f, 1.f, 1.f)},
        {0.f, 0.33f, 0.66f, 1.f},
        densityScale,
        anisotropy,
        gradientShadingScale));
  }

  if (addTorusVolume) {
    volumetricModels.emplace_back(createProceduralVolumetricModel(
        [](const vec3f &p) { return turbulentTorus(p, 1.f, 0.375f); },
        {vec3f(0.0, 0.0, 0.0),
            vec3f(1.0, 0.65, 0.0),
            vec3f(0.12, 0.6, 1.0),
            vec3f(1.0, 1.0, 1.0)},
        {0.f, 0.33f, 0.66f, 1.f},
        densityScale,
        anisotropy,
        gradientShadingScale));
  }

  for (auto volumetricModel : volumetricModels)
    volumetricModel.commit();

  std::vector<cpp::GeometricModel> geometricModels;

  if (addBoxes) {
    auto box1 = makeBoxGeometry(
        box3f(vec3f(-1.5f, -1.f, -0.75f), vec3f(-0.5f, 0.f, 0.25f)));
    auto box2 =
        makeBoxGeometry(box3f(vec3f(0.0f, -1.5f, 0.f), vec3f(2.f, 1.5f, 2.f)));

    geometricModels.emplace_back(
        createGeometricModel(box1, rendererType, vec3f(0.2f, 0.2f, 0.2f)));
    geometricModels.emplace_back(
        createGeometricModel(box2, rendererType, vec3f(0.2f, 0.2f, 0.2f)));
  }

  for (auto geometricModel : geometricModels)
    geometricModel.commit();

  if (!volumetricModels.empty())
    group.setParam("volume", cpp::CopiedData(volumetricModels));

  if (!geometricModels.empty())
    group.setParam("geometry", cpp::CopiedData(geometricModels));

  group.commit();

  return group;
}

cpp::World PerlinNoiseVolumes::buildWorld() const
{
  // Skip clipping if not enabled
  std::vector<cpp::Instance> instances;
  if (withClipping) {
    // Create clipping sphere
    cpp::Geometry sphereGeometry("sphere");
    std::vector<vec3f> position = {vec3f(-.5f, .2f, -.5f)};
    sphereGeometry.setParam("sphere.position", cpp::CopiedData(position));
    sphereGeometry.setParam("radius", 1.2f);
    sphereGeometry.commit();

    cpp::GeometricModel model(sphereGeometry);
    model.commit();

    cpp::Group group;
    group.setParam("clippingGeometry", cpp::CopiedData(model));
    group.commit();

    cpp::Instance inst(group);
    inst.commit();
    instances.push_back(inst);
  }

  auto world = Builder::buildWorld(instances);

  std::vector<cpp::Light> lightHandles;

  cpp::Light quadLight("quad");
  quadLight.setParam("position", vec3f(-4.0f, 3.0f, 1.0f));
  quadLight.setParam("edge1", vec3f(0.f, 0.0f, -1.0f));
  quadLight.setParam("edge2", vec3f(1.0f, 0.5, 0.0f));
  quadLight.setParam("intensity", 50.0f);
  quadLight.setParam("color", vec3f(2.6f, 2.5f, 2.3f));
  quadLight.commit();

  cpp::Light ambientLight("ambient");
  ambientLight.setParam("intensity", 0.4f);
  ambientLight.setParam("color", vec3f(1.f));
  ambientLight.setParam("visible", false);
  ambientLight.commit();

  if (addAreaLight)
    lightHandles.push_back(quadLight);
  if (addAmbientLight)
    lightHandles.push_back(ambientLight);

  if (lightHandles.empty())
    world.removeParam("light");
  else
    world.setParam("light", cpp::CopiedData(lightHandles));

  return world;
}

OSP_REGISTER_TESTING_BUILDER(PerlinNoiseVolumes, perlin_noise_volumes);
OSP_REGISTER_TESTING_BUILDER(
    PerlinNoiseVolumes(true), clip_perlin_noise_volumes);
OSP_REGISTER_TESTING_BUILDER(
    PerlinNoiseVolumes(false, true), perlin_noise_volumes_gradient);
OSP_REGISTER_TESTING_BUILDER(
    PerlinNoiseVolumes(true, true), clip_perlin_noise_volumes_gradient);

} // namespace testing
} // namespace ospray
