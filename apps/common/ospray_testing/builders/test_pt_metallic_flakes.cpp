// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_pt_material.h"

namespace ospray {
namespace testing {

struct PtMetallicFlakes : public PtMaterial
{
  PtMetallicFlakes() = default;
  ~PtMetallicFlakes() override = default;

  std::vector<cpp::Material> buildMaterials() const override;
};

static cpp::Material makeMetallicFlakesMaterial(
    float flakeAmount, float flakeSpread)
{
  cpp::Material mat("metallicPaint");
  mat.setParam("baseColor", vec3f(1.f, 0.f, 0.f));
  mat.setParam("flakeAmount", flakeAmount);
  mat.setParam("flakeSpread", flakeSpread);
  mat.commit();

  return mat;
}

// Inlined definitions ////////////////////////////////////////////////////

std::vector<cpp::Material> PtMetallicFlakes::buildMaterials() const
{
  std::vector<cpp::Material> materials;

  constexpr int dimSize = 5;
  index_sequence_2D numSpheres(dimSize);
  for (auto i : numSpheres) {
    auto i_f = static_cast<vec2f>(i);
    materials.push_back(
        makeMetallicFlakesMaterial(lerp(i_f.x / (dimSize - 1), 1.f, 0.f),
            lerp(i_f.y / (dimSize - 1), 0.f, 1.f)));
  }

  return materials;
}

OSP_REGISTER_TESTING_BUILDER(PtMetallicFlakes, test_pt_metallic_flakes);

} // namespace testing
} // namespace ospray
