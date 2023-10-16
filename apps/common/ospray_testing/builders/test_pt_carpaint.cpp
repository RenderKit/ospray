// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_pt_material.h"

namespace ospray {
namespace testing {

struct PtCarPaint : public PtMaterial
{
  PtCarPaint() = default;
  ~PtCarPaint() override = default;

  std::vector<cpp::Material> buildMaterials() const override;
};

static cpp::Material makeCarPaintMaterial(
    std::initializer_list<std::pair<std::string, float>> params)
{
  cpp::Material mat("carPaint");

  mat.setParam("baseColor", vec3f(0.0177f, 0.189f, 0.590f));
  mat.setParam("flakeColor", vec3f(0.277f, 0.717f, 0.990f));
  mat.setParam("flipflopColor", vec3f(0.252f, 0.0385f, 0.550f));

  for (auto &param : params)
    mat.setParam(param.first, param.second);

  mat.commit();
  return mat;
}

// Inlined definitions ////////////////////////////////////////////////////

std::vector<cpp::Material> PtCarPaint::buildMaterials() const
{
  std::vector<cpp::Material> materials;
  constexpr int dimSize = 5;
  index_sequence_2D numSpheres(dimSize);

  // flakeDensity
  for (int i = 0; i < dimSize; i++)
    materials.push_back(makeCarPaintMaterial(
        {{"flakeDensity", float(i) / (dimSize - 1)}, {"flakeScale", 1000.f}}));

  // flipflopFalloff
  for (int i = 0; i < dimSize; i++)
    materials.push_back(makeCarPaintMaterial({{"flakeDensity", 1.f},
        {"flakeScale", 1000.f},
        {"flipflopFalloff", 1.f - float(i) / (dimSize - 1)}}));

  // flakeScale
  for (int i = 0; i < dimSize; i++)
    materials.push_back(makeCarPaintMaterial({{"flakeDensity", 1.f},
        {"flakeScale", lerp(float(i) / (dimSize - 1), 1000.f, 100.f)}}));

  // flakeRoughness
  for (int i = 0; i < dimSize; i++)
    materials.push_back(makeCarPaintMaterial({{"flakeDensity", 1.f},
        {"flakeScale", 1000.f},
        {"flakeRoughness", float(i) / (dimSize - 1)}}));

  // coat
  for (int i = 0; i < dimSize; i++)
    materials.push_back(
        makeCarPaintMaterial({{"coat", float(i) / (dimSize - 1)}}));

  return materials;
}

OSP_REGISTER_TESTING_BUILDER(PtCarPaint, test_pt_carpaint);

} // namespace testing
} // namespace ospray
