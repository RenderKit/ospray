// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_pt_material.h"

using namespace rkcommon::math;

namespace ospray {
namespace testing {

struct PtLuminous : public PtMaterial
{
  PtLuminous() = default;
  ~PtLuminous() override = default;

  std::vector<cpp::Material> buildMaterials() const override;
};

// Inlined definitions ////////////////////////////////////////////////////

std::vector<cpp::Material> PtLuminous::buildMaterials() const
{
  std::vector<cpp::Material> materials;
  constexpr int dimSize = 5;
  index_sequence_2D numSpheres(dimSize);

  for (auto i : numSpheres) {
    auto i_f = static_cast<vec2f>(i);
    materials.push_back(cpp::Material("luminous"));
    materials.back().setParam("color", vec3f(0.7f, 0.7f, 1.f));
    materials.back().setParam("intensity", i_f.x / (dimSize - 1));
    materials.back().setParam("transparency", 1.f - i_f.y / (dimSize - 1));
    materials.back().commit();
  }

  return materials;
}

OSP_REGISTER_TESTING_BUILDER(PtLuminous, test_pt_luminous);

} // namespace testing
} // namespace ospray
