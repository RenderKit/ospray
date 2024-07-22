// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Builder.h"
#include "ospray_testing.h"

#include "rkcommon/utility/multidim_index_sequence.h"

using namespace rkcommon::math;

namespace ospray {
namespace testing {

struct MipMapTextures : public detail::Builder
{
  MipMapTextures() = default;
  ~MipMapTextures() override = default;

  void commit() override;

  cpp::Group buildGroup() const override;

 private:
  float scale{1.0f};
  OSPTextureFilter filter{OSP_TEXTURE_FILTER_LINEAR};
};

// Inlined definitions ////////////////////////////////////////////////////

void MipMapTextures::commit()
{
  Builder::commit();

  scale = getParam<float>("tcScale", 1.0f);
  filter = static_cast<OSPTextureFilter>(
      getParam<uint32_t>("filter", OSP_TEXTURE_FILTER_LINEAR));
  addPlane = false;
}

cpp::Group MipMapTextures::buildGroup() const
{
  std::vector<vec3f> v_position = {{-2.5f, -1.5f, -4.f},
      {2.5f, -1.5f, -4.f},
      {2.5f, 1.5f, 0.f},
      {-2.5f, 1.5f, 0.f},
      {-1.25f, 1.5f, -2.f},
      {1.25f, 1.5f, -2.f},
      {1.25f, 3.5f, -0.5f},
      {-1.25f, 3.5f, -0.5f}};
  std::vector<vec2f> v_texcoord = {{0.f, 0.f},
      {scale, 0.f},
      {scale, scale},
      {0.f, scale},
      {0.f, 0.f},
      {scale, 0.f},
      {scale, scale},
      {0.f, scale}};

  cpp::Geometry planeGeometry("mesh");
  planeGeometry.setParam("vertex.position", cpp::CopiedData(v_position));
  planeGeometry.setParam("vertex.texcoord", cpp::CopiedData(v_texcoord));
  planeGeometry.setParam("quadSoup", true);
  planeGeometry.commit();

  vec2ul texSize(500, 500);
  std::vector<uint8_t> texData(texSize.product());
  uint8_t grayTexel{50};
  uint8_t lightTexel{200};
  for (size_t i = 0; i < texSize.x; i++) {
    const int sz = 1 + i * 4 / texSize.x;
    for (size_t j = 0; j < texSize.y; j++) {
      texData[j * texSize.x + i] =
          (i / sz + j / sz) % 2 ? lightTexel : grayTexel;
    }
  }

  cpp::Texture procTex("texture2d");
  procTex.setParam("format", OSP_TEXTURE_L8);
  procTex.setParam("filter", filter);
  procTex.setParam("data", cpp::CopiedData(texData.data(), texSize));
  procTex.commit();

  cpp::GeometricModel plane(planeGeometry);

  cpp::Material material("obj");
  material.setParam("map_kd", procTex);
  material.setParam("map_kd.scale", vec2f(scale));
  material.setParam("map_kd.translation", vec2f(0.5f - 0.5f / scale));
  material.commit();

  plane.setParam("material", material);
  plane.commit();

  cpp::Group planeGroup;
  planeGroup.setParam("geometry", cpp::CopiedData(plane));
  planeGroup.commit();

  return planeGroup;
}

OSP_REGISTER_TESTING_BUILDER(MipMapTextures, mip_map_textures);

} // namespace testing
} // namespace ospray
