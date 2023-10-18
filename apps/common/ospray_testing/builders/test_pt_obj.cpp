// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_pt_material.h"

namespace ospray {
namespace testing {

struct PtObj : public PtMaterial
{
  PtObj() = default;
  ~PtObj() override = default;

  std::vector<cpp::Material> buildMaterials() const override;
};

static cpp::Material makeObjMaterial(
    float d, vec3f Kd, vec3f Ks, float Ns, vec3f Tf)
{
  cpp::Material mat("obj");
  mat.setParam("d", d);
  mat.setParam("kd", Kd);
  mat.setParam("ks", Ks);
  mat.setParam("ns", Ns);
  mat.setParam("tf", Tf);
  mat.commit();

  return mat;
}

// Inlined definitions ////////////////////////////////////////////////////

std::vector<cpp::Material> PtObj::buildMaterials() const
{
  std::vector<cpp::Material> materials;
  constexpr int dimSize = 5;
  index_sequence_2D numSpheres(dimSize);

  // d
  for (int i = 0; i < dimSize; i++)
    materials.push_back(makeObjMaterial(1.f - float(i) / (dimSize - 1),
        vec3f(.8f, 0.f, 0.f),
        vec3f(0.f),
        10.f,
        vec3f(0.f)));

  // Kd
  for (int i = 0; i < dimSize; i++)
    materials.push_back(makeObjMaterial(1.f,
        vec3f(1.f - float(i) / (dimSize - 1), 0.f, 0.f),
        vec3f(0.f),
        10.f,
        vec3f(0.f)));

  // Ks
  for (int i = 0; i < dimSize; i++)
    materials.push_back(makeObjMaterial(1.f,
        vec3f(.8f, 0.f, 0.f),
        vec3f(float(i) / (dimSize - 1)),
        1000.f,
        vec3f(0.f)));

  // Ns
  for (int i = 0; i < dimSize; i++)
    materials.push_back(makeObjMaterial(1.f,
        vec3f(.8f, 0.f, 0.f),
        vec3f(1.f),
        lerp(float(i) / (dimSize - 1), 10.f, 1000.f),
        vec3f(0.f)));

  // Tf
  for (int i = 0; i < dimSize; i++)
    materials.push_back(makeObjMaterial(1.f,
        vec3f(.2f, 0.f, 0.f),
        vec3f(.2f, 0.f, 0.f),
        1000.f,
        (float(i) / (dimSize - 1)) * vec3f(1.f, .5f, .5f)));

  return materials;
}

OSP_REGISTER_TESTING_BUILDER(PtObj, test_pt_obj);

} // namespace testing
} // namespace ospray
