// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Builder.h"
#include "Noise.h"
#include "ospray_testing.h"
#include "rkcommon/tasking/parallel_for.h"

using namespace rkcommon::math;

namespace ospray {
namespace testing {

struct VdbVolume : public detail::Builder
{
  VdbVolume() = default;
  ~VdbVolume() override = default;

  void commit() override;

  cpp::Group buildGroup() const override;
  cpp::World buildWorld() const override;

 private:
  float densityScale{1.f};
  float anisotropy{0.f};
};

// Inlined definitions ////////////////////////////////////////////////////

void VdbVolume::commit()
{
  Builder::commit();

  densityScale = getParam<float>("densityScale", 20.f);
  anisotropy = getParam<float>("anisotropy", 0.f);
}

cpp::Group VdbVolume::buildGroup() const
{
  cpp::Volume volume("vdb");

  std::vector<uint32_t> level;
  std::vector<vec3i> origin;
  std::vector<cpp::CopiedData> data;
  std::vector<uint32_t> format;

  constexpr uint32_t leafRes = 8;
  constexpr uint32_t domainRes = 128;
  constexpr uint32_t N = domainRes / leafRes;
  constexpr size_t numLeaves = N * static_cast<size_t>(N) * N;

  level.reserve(numLeaves);
  origin.reserve(numLeaves);
  data.reserve(numLeaves);
  format.reserve(numLeaves);

  std::vector<float> leaf(leafRes * (size_t)leafRes * leafRes, 0.f);
  for (uint32_t z = 0; z < N; ++z)
    for (uint32_t y = 0; y < N; ++y)
      for (uint32_t x = 0; x < N; ++x) {
        range1f leafValueRange;
        tasking::parallel_for(leafRes, [&](uint32_t vz) {
          for (uint32_t vy = 0; vy < leafRes; ++vy)
            for (uint32_t vx = 0; vx < leafRes; ++vx) {
              float v = 0.f;
              const vec3f p((x * leafRes + vx + 0.5f) / (float)domainRes,
                  (y * leafRes + vy + 0.5f) / (float)domainRes,
                  (z * leafRes + vz + 0.5f) / (float)domainRes);
              if (turbulentTorus(p, 0.75f, 0.375f))
                v = 0.5f + 0.5f * PerlinNoise::noise(p, 12);

              const size_t idx =
                  vx * (size_t)leafRes * leafRes + vy * (size_t)leafRes + vz;
              leafValueRange.extend(v);
              leaf.at(idx) = v;
            }
        });

        if (leafValueRange.lower != 0.f || leafValueRange.upper != 0.f) {
          level.push_back(3);
          origin.push_back(vec3i(x * leafRes, y * leafRes, z * leafRes));
          data.emplace_back(
              cpp::CopiedData(leaf.data(), vec3ul(leafRes, leafRes, leafRes)));
          format.push_back(1);
        }
      }

  if (level.empty())
    throw std::runtime_error("vdb volume is empty.");

  volume.setParam("filter", (int)OSP_VOLUME_FILTER_TRILINEAR);
  volume.setParam("gradientFilter", (int)OSP_VOLUME_FILTER_TRILINEAR);
  volume.setParam("node.level", cpp::CopiedData(level));
  volume.setParam("node.origin", cpp::CopiedData(origin));
  volume.setParam("node.data", cpp::CopiedData(data));
  volume.setParam("node.format", cpp::CopiedData(format));

  std::vector<float> i2o = {8.f / domainRes,
      0,
      0,
      0,
      8.f / domainRes,
      0,
      0,
      0,
      8.f / domainRes,
      -4.f,
      0,
      -4.f};
 // volume.setParam("indexToObject", cpp::CopiedData(i2o));

  volume.commit();

  cpp::VolumetricModel model(volume);
  model.setParam("transferFunction", makeTransferFunction({0.f, 1.f}));
  model.setParam("densityScale", densityScale);
  model.setParam("anisotropy", anisotropy);
  model.commit();

  cpp::Group group;

  group.setParam("volume", cpp::CopiedData(model));
  group.commit();

  return group;
}

cpp::World VdbVolume::buildWorld() const
{
  std::vector<cpp::Instance> instances;
  auto world = Builder::buildWorld(instances);

  std::vector<cpp::Light> lightHandles;

  cpp::Light quadLight("quad");
  quadLight.setParam("position", vec3f(-4.f, 8.0f, 4.f));
  quadLight.setParam("edge1", vec3f(0.f, 0.0f, -8.0f));
  quadLight.setParam("edge2", vec3f(2.f, 1.0f, 0.0f));
  quadLight.setParam("intensity", 5.0f);
  quadLight.setParam("color", vec3f(2.8f, 2.2f, 1.9f));
  quadLight.commit();
  lightHandles.push_back(quadLight);
  world.setParam("light", cpp::CopiedData(lightHandles));

  return world;
}

OSP_REGISTER_TESTING_BUILDER(VdbVolume, vdb_volume);

} // namespace testing
} // namespace ospray
