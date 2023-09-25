// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_pt_material.h"

namespace ospray {
namespace testing {

struct PtPrincipledMetal : public PtMaterial
{
  PtPrincipledMetal() = default;
  ~PtPrincipledMetal() override = default;

  std::vector<cpp::Material> buildMaterials() const override;
};

struct PtPrincipledPlastic : public PtMaterial
{
  PtPrincipledPlastic() = default;
  ~PtPrincipledPlastic() override = default;

  std::vector<cpp::Material> buildMaterials() const override;
};

struct PtPrincipledGlass : public PtMaterial
{
  PtPrincipledGlass() = default;
  ~PtPrincipledGlass() override = default;

  std::vector<cpp::Material> buildMaterials() const override;
};

static cpp::Material makePrincipledMetalMaterial(vec3f baseColor,
    float metallic,
    float roughness,
    float anisotropy,
    float coat,
    float coatRoughness)
{
  cpp::Material mat("principled");
  mat.setParam("baseColor", baseColor);
  mat.setParam("metallic", metallic);
  mat.setParam("roughness", roughness);
  mat.setParam("anisotropy", anisotropy);
  mat.setParam("coat", coat);
  mat.setParam("coatRoughness", coatRoughness);
  mat.commit();

  return mat;
}

static cpp::Material makePrincipledPlasticMaterial(vec3f baseColor,
    float ior,
    float specular,
    float sheen,
    float sheenTint,
    float sheenRoughness,
    float opacity)
{
  cpp::Material mat("principled");
  mat.setParam("baseColor", baseColor);
  mat.setParam("ior", ior);
  mat.setParam("specular", specular);
  mat.setParam("sheen", sheen);
  mat.setParam("sheenTint", sheenTint);
  mat.setParam("sheenRoughness", sheenRoughness);
  mat.setParam("opacity", opacity);
  mat.commit();

  return mat;
}

static cpp::Material makePrincipledGlassMaterial(vec3f baseColor,
    float transmission,
    float roughness,
    float ior,
    float transmissionDepth,
    bool thin,
    float thickness,
    float backlight)
{
  cpp::Material mat("principled");
  mat.setParam("baseColor", baseColor);
  mat.setParam("transmissionColor", baseColor);
  mat.setParam("transmission", transmission);
  mat.setParam("roughness", roughness);
  mat.setParam("ior", ior);
  mat.setParam("transmissionDepth", transmissionDepth);
  mat.setParam("thin", thin);
  mat.setParam("thickness", thickness);
  mat.setParam("backlight", backlight);
  mat.commit();

  return mat;
}

// Inlined definitions ////////////////////////////////////////////////////

std::vector<cpp::Material> PtPrincipledMetal::buildMaterials() const
{
  std::vector<cpp::Material> materials;
  constexpr int dimSize = 5;
  index_sequence_2D numSpheres(dimSize);

  // metallic
  for (int i = 0; i < dimSize; i++) {
    materials.push_back(
        makePrincipledMetalMaterial(vec3f(0.0261f, 0.195f, 0.870f),
            float(i) / (dimSize - 1),
            0.f,
            0.f,
            0.f,
            0.f));
  }

  // roughness
  for (int i = 0; i < dimSize; i++) {
    materials.push_back(
        makePrincipledMetalMaterial(vec3f(0.0261f, 0.195f, 0.870f),
            1.f,
            float(i) / (dimSize - 1),
            0.f,
            0.f,
            0.f));
  }

  // anisotropy
  for (int i = 0; i < dimSize; i++) {
    materials.push_back(
        makePrincipledMetalMaterial(vec3f(0.0261f, 0.195f, 0.870f),
            1.f,
            .5f,
            float(i) / (dimSize - 1),
            0.f,
            0.f));
  }

  // coat
  for (int i = 0; i < dimSize; i++) {
    materials.push_back(
        makePrincipledMetalMaterial(vec3f(0.860f, 0.0258f, 0.0536f),
            1.f,
            .5f,
            0.f,
            float(i) / (dimSize - 1),
            0.f));
  }

  // coatRoughness
  for (int i = 0; i < dimSize; i++) {
    materials.push_back(
        makePrincipledMetalMaterial(vec3f(0.860f, 0.0258f, 0.0536f),
            1.f,
            .5f,
            0.f,
            1.f,
            float(i) / (dimSize - 1)));
  }

  return materials;
}

std::vector<cpp::Material> PtPrincipledPlastic::buildMaterials() const
{
  std::vector<cpp::Material> materials;
  constexpr int dimSize = 5;
  index_sequence_2D numSpheres(dimSize);

  // specular
  for (int i = 0; i < dimSize; i++) {
    materials.push_back(makePrincipledPlasticMaterial(vec3f(0.770, 0.00, 0.00),
        1.45f,
        float(i) / (dimSize - 1),
        0.f,
        0.f,
        0.f,
        1.f));
  }

  // sheen
  for (int i = 0; i < dimSize; i++) {
    materials.push_back(
        makePrincipledPlasticMaterial(vec3f(0.950f, 0.494f, 0.0380f),
            1.f,
            0.f,
            float(i) / (dimSize - 1),
            0.f,
            .5f,
            1.f));
  }

  // sheenTint
  for (int i = 0; i < dimSize; i++) {
    materials.push_back(
        makePrincipledPlasticMaterial(vec3f(0.950f, 0.494f, 0.0380f),
            1.f,
            0.f,
            1.f,
            float(i) / (dimSize - 1),
            .5f,
            1.f));
  }

  // sheenRoughness
  for (int i = 0; i < dimSize; i++) {
    materials.push_back(
        makePrincipledPlasticMaterial(vec3f(0.950f, 0.494f, 0.0380f),
            1.f,
            0.f,
            1.f,
            0.f,
            float(i) / (dimSize - 1),
            1.f));
  }

  // opacity
  for (int i = 0; i < dimSize; i++) {
    materials.push_back(
        makePrincipledPlasticMaterial(vec3f(0.930f, 0.884f, 0.0186f),
            1.45f,
            1.f,
            0.f,
            0.f,
            0.f,
            1.f - float(i) / (dimSize - 1)));
  }

  return materials;
}

std::vector<cpp::Material> PtPrincipledGlass::buildMaterials() const
{
  std::vector<cpp::Material> materials;
  constexpr int dimSize = 5;
  index_sequence_2D numSpheres(dimSize);

  // transmission
  for (int i = 0; i < dimSize; i++) {
    materials.push_back(
        makePrincipledGlassMaterial(vec3f(0.0400f, 0.680f, 1.00f),
            float(i) / (dimSize - 1),
            0.f,
            1.1f,
            1.f,
            false,
            1.f,
            0.f));
  }

  // roughness
  for (int i = 0; i < dimSize; i++) {
    materials.push_back(
        makePrincipledGlassMaterial(vec3f(0.0400f, 0.680f, 1.00f),
            1.f,
            float(i) / (dimSize - 1),
            1.5f,
            1.f,
            false,
            1.f,
            0.f));
  }

  // ior
  for (int i = 0; i < dimSize; i++) {
    materials.push_back(
        makePrincipledGlassMaterial(vec3f(0.0400f, 0.680f, 1.00f),
            1.f,
            0.f,
            lerp(float(i) / (dimSize - 1), 1.f, 2.f),
            1.f,
            false,
            1.f,
            0.f));
  }

  // transmissionDepth
  for (int i = 0; i < dimSize; i++) {
    materials.push_back(
        makePrincipledGlassMaterial(vec3f(0.0400f, 0.680f, 1.00f),
            1.f,
            0.f,
            1.5f,
            lerp(float(i) / (dimSize - 1), 2.f, 0.f),
            false,
            1.f,
            0.f));
  }

  // backlight
  for (int i = 0; i < dimSize; i++) {
    materials.push_back(
        makePrincipledGlassMaterial(vec3f(0.0400f, 0.200f, 1.00f),
            .3f,
            0.f,
            1.5f,
            1.f,
            true,
            1.f,
            lerp(float(i) / (dimSize - 1), 0.f, 2.f)));
  }

  return materials;
}

OSP_REGISTER_TESTING_BUILDER(PtPrincipledMetal, test_pt_principled_metal);
OSP_REGISTER_TESTING_BUILDER(PtPrincipledPlastic, test_pt_principled_plastic);
OSP_REGISTER_TESTING_BUILDER(PtPrincipledGlass, test_pt_principled_glass);

} // namespace testing
} // namespace ospray
