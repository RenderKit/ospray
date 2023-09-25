// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_pt_material.h"

namespace ospray {
namespace testing {

struct PtPlastic : public PtMaterial
{
  PtPlastic() = default;
  ~PtPlastic() override = default;

  std::vector<cpp::Material> buildMaterials() const override;
};

static cpp::Material makePlasticMaterial(
    vec3f color, float eta, float roughness)
{
  cpp::Material mat("plastic");
  mat.setParam("pigmentColor", color);
  mat.setParam("eta", eta);
  mat.setParam("roughness", roughness);
  mat.commit();

  return mat;
}

// Inlined definitions ////////////////////////////////////////////////////

std::vector<cpp::Material> PtPlastic::buildMaterials() const
{
  std::vector<cpp::Material> materials;

  constexpr int dimSize = 5;
  index_sequence_2D numSpheres(dimSize);
  for (auto i : numSpheres) {
    auto i_f = static_cast<vec2f>(i);
    materials.push_back(makePlasticMaterial(vec3f(.8f, 0.f, 0.f),
        lerp(i_f.y / (dimSize - 1), 1.f, 2.f),
        i_f.x / (dimSize - 1)));
  }

  return materials;
}

OSP_REGISTER_TESTING_BUILDER(PtPlastic, test_pt_plastic);

} // namespace testing
} // namespace ospray
