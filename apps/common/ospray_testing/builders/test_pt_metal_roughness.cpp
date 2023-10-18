// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_pt_material.h"

namespace ospray {
namespace testing {

struct PtMetalRoughness : public PtMaterial
{
  PtMetalRoughness() = default;
  ~PtMetalRoughness() override = default;

  std::vector<cpp::Material> buildMaterials() const override;
};

static cpp::Material makeMetalMaterial(vec3f eta, vec3f k, float roughness)
{
  cpp::Material mat("metal");
  mat.setParam("eta", eta);
  mat.setParam("k", k);
  mat.setParam("roughness", roughness);
  mat.commit();

  return mat;
}

// Inlined definitions ////////////////////////////////////////////////////

std::vector<cpp::Material> PtMetalRoughness::buildMaterials() const
{
  std::vector<cpp::Material> materials;
  constexpr int dimSize = 5;
  index_sequence_2D numSpheres(dimSize);

  // Silver
  for (int i = 0; i < dimSize; ++i) {
    materials.push_back(makeMetalMaterial(vec3f(0.051f, 0.043f, 0.041f),
        vec3f(5.3f, 3.6f, 2.3f),
        float(i) / (dimSize - 1)));
  }

  // Aluminium
  for (int i = 0; i < dimSize; ++i) {
    materials.push_back(makeMetalMaterial(vec3f(1.5f, 0.98f, 0.6f),
        vec3f(7.6f, 6.6f, 5.4f),
        float(i) / (dimSize - 1)));
  }

  // Gold
  for (int i = 0; i < dimSize; ++i) {
    materials.push_back(makeMetalMaterial(vec3f(0.07f, 0.37f, 1.5f),
        vec3f(3.7f, 2.3f, 1.7f),
        float(i) / (dimSize - 1)));
  }

  // Chromium
  for (int i = 0; i < dimSize; ++i) {
    materials.push_back(makeMetalMaterial(vec3f(3.2f, 3.1f, 2.3f),
        vec3f(3.3f, 3.3f, 3.1f),
        float(i) / (dimSize - 1)));
  }

  // Copper
  for (int i = 0; i < dimSize; ++i) {
    materials.push_back(makeMetalMaterial(vec3f(0.1f, 0.8f, 1.1f),
        vec3f(3.5f, 2.5f, 2.4f),
        float(i) / (dimSize - 1)));
  }

  return materials;
}

OSP_REGISTER_TESTING_BUILDER(PtMetalRoughness, test_pt_metal_roughness);

} // namespace testing
} // namespace ospray
