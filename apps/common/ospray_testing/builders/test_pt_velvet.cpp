// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_pt_material.h"

namespace ospray {
namespace testing {

struct PtVelvet : public PtMaterial
{
  PtVelvet() = default;
  ~PtVelvet() override = default;

  std::vector<cpp::Material> buildMaterials() const override;
};

static cpp::Material makeVelvetMaterial(
    float backScattering, float horizonScatteringFallOff)
{
  cpp::Material mat("velvet");
  mat.setParam("backScattering", backScattering);
  mat.setParam("horizonScatteringFallOff", horizonScatteringFallOff);
  mat.commit();

  return mat;
}

// Inlined definitions ////////////////////////////////////////////////////

std::vector<cpp::Material> PtVelvet::buildMaterials() const
{
  std::vector<cpp::Material> materials;

  constexpr int dimSize = 5;
  index_sequence_2D numSpheres(dimSize);
  for (auto i : numSpheres) {
    auto i_f = static_cast<vec2f>(i);
    materials.push_back(makeVelvetMaterial(
        i_f.x / (dimSize - 1), lerp(i_f.y / (dimSize - 1), 0.f, 20.f)));
  }

  return materials;
}

OSP_REGISTER_TESTING_BUILDER(PtVelvet, test_pt_velvet);

} // namespace testing
} // namespace ospray
