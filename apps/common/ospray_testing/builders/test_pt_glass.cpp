// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_pt_material.h"

namespace ospray {
namespace testing {

struct PtGlass : public PtMaterial
{
  PtGlass() = default;
  ~PtGlass() override = default;

  std::vector<cpp::Material> buildMaterials() const override;
};

static cpp::Material makeGlassMaterial(
    float eta, vec3f color, float attenuationDistance)
{
  cpp::Material mat("glass");
  mat.setParam("eta", eta);
  mat.setParam("attenuationColor", color);
  mat.setParam("attenuationDistance", attenuationDistance);
  mat.commit();

  return mat;
}

// Inlined definitions ////////////////////////////////////////////////////

std::vector<cpp::Material> PtGlass::buildMaterials() const
{
  std::vector<cpp::Material> materials;

  constexpr int dimSize = 5;
  index_sequence_2D numSpheres(dimSize);
  for (auto i : numSpheres) {
    auto i_f = static_cast<vec2f>(i);
    materials.push_back(makeGlassMaterial(lerp(i_f.x / (dimSize - 1), 1.f, 2.f),
        vec3f(.1f, .5f, 1.f),
        lerp(1.f - i_f.y / (dimSize - 1), 0.5f, 5.f)));
  }

  return materials;
}

OSP_REGISTER_TESTING_BUILDER(PtGlass, test_pt_glass);

} // namespace testing
} // namespace ospray
