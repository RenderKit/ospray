// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_pt_material.h"

namespace ospray {
namespace testing {

struct PtAlloyRoughness : public PtMaterial
{
  PtAlloyRoughness() = default;
  ~PtAlloyRoughness() override = default;

  std::vector<cpp::Material> buildMaterials() const override;
};

static cpp::Material makeAlloyMaterial(
    vec3f color, vec3f edgeColor, float roughness)
{
  cpp::Material mat("alloy");
  mat.setParam("color", color);
  mat.setParam("edgeColor", edgeColor);
  mat.setParam("roughness", roughness);
  mat.commit();

  return mat;
}

// Inlined definitions ////////////////////////////////////////////////////

std::vector<cpp::Material> PtAlloyRoughness::buildMaterials() const
{
  std::vector<cpp::Material> materials;
  constexpr int dimSize = 5;
  index_sequence_2D numSpheres(dimSize);

  // Silver
  for (int i = 0; i < dimSize; ++i) {
    materials.push_back(makeAlloyMaterial(vec3f(.66f, .66f, .67f),
        vec3f(1.f, 1.f, 1.f),
        float(i) / (dimSize - 1)));
  }

  // Aluminium
  for (int i = 0; i < dimSize; ++i) {
    materials.push_back(makeAlloyMaterial(vec3f(.52f, .52f, .5f),
        vec3f(1.f, 1.f, 1.f),
        float(i) / (dimSize - 1)));
  }

  // Gold
  for (int i = 0; i < dimSize; ++i) {
    materials.push_back(makeAlloyMaterial(vec3f(.83f, .68f, .21f),
        vec3f(1.f, 1.f, 1.f),
        float(i) / (dimSize - 1)));
  }

  // Chromium
  for (int i = 0; i < dimSize; ++i) {
    materials.push_back(makeAlloyMaterial(vec3f(.65f, .66f, .67f),
        vec3f(1.f, 1.f, 1.f),
        float(i) / (dimSize - 1)));
  }

  // Copper
  for (int i = 0; i < dimSize; ++i) {
    materials.push_back(makeAlloyMaterial(vec3f(.72f, .45f, .2f),
        vec3f(1.f, 1.f, 1.f),
        float(i) / (dimSize - 1)));
  }

  return materials;
}

OSP_REGISTER_TESTING_BUILDER(PtAlloyRoughness, test_pt_alloy_roughness);

} // namespace testing
} // namespace ospray
