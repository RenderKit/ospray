// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_pt_tex.h"

namespace ospray {
namespace testing {

struct PtMaterialTex : public PtTex
{
  PtMaterialTex() = default;
  ~PtMaterialTex() override = default;

  std::vector<cpp::Material> buildMaterials(
      const cpp::Texture &texR, const cpp::Texture &texRGBA) const override;
};

// Inlined definitions ////////////////////////////////////////////////////

std::vector<cpp::Material> PtMaterialTex::buildMaterials(
    const cpp::Texture &texR, const cpp::Texture &texRGBA) const
{
  std::vector<cpp::Material> materials;

  // carPaint
  {
    // roughness
    materials.push_back(cpp::Material("carPaint"));
    materials.back().setParam("baseColor", vec3f(0.0177f, 0.189f, 0.590f));
    setTexture<float>(materials.back(), "roughness", texR);

    // flakeColor
    materials.push_back(cpp::Material("carPaint"));
    materials.back().setParam("baseColor", vec3f(0.0177f, 0.189f, 0.590f));
    materials.back().setParam("flakeScale", 2000.f);
    materials.back().setParam("flakeDensity", 1.f);
    setTexture<vec3f>(materials.back(), "flakeColor", texRGBA);

    // flakeDensity
    materials.push_back(cpp::Material("carPaint"));
    materials.back().setParam("baseColor", vec3f(0.0177f, 0.189f, 0.590f));
    materials.back().setParam("flakeColor", vec3f(0.277f, 0.717f, 0.990f));
    materials.back().setParam("flakeScale", 2000.f);
    setTexture<float>(materials.back(), "flakeDensity", texR);

    // flakeRoughness
    materials.push_back(cpp::Material("carPaint"));
    materials.back().setParam("baseColor", vec3f(0.0177f, 0.189f, 0.590f));
    materials.back().setParam("flakeDensity", 1.f);
    materials.back().setParam("flakeColor", vec3f(0.277f, 0.717f, 0.990f));
    materials.back().setParam("flakeScale", 2000.f);
    setTexture<float>(materials.back(), "flakeRoughness", texR);

    // flipflopFalloff
    materials.push_back(cpp::Material("carPaint"));
    materials.back().setParam("baseColor", vec3f(0.0177f, 0.189f, 0.590f));
    materials.back().setParam("flakeDensity", 1.f);
    materials.back().setParam("flakeColor", vec3f(0.277f, 0.717f, 0.990f));
    materials.back().setParam("flakeScale", 1000.f);
    materials.back().setParam("flipflopColor", vec3f(0.252f, 0.0385f, 0.550f));
    setTexture<float>(materials.back(), "flipflopFalloff", texR);

    // coat
    materials.push_back(cpp::Material("carPaint"));
    materials.back().setParam("baseColor", vec3f(0.0177f, 0.189f, 0.590f));
    setTexture<float>(materials.back(), "coat", texR);

    // coatColor
    materials.push_back(cpp::Material("carPaint"));
    materials.back().setParam("baseColor", vec3f(0.0177f, 0.189f, 0.590f));
    setTexture<vec3f>(materials.back(), "coatColor", texRGBA);

    // coatRoughness
    materials.push_back(cpp::Material("carPaint"));
    materials.back().setParam("baseColor", vec3f(0.0177f, 0.189f, 0.590f));
    setTexture<float>(materials.back(), "coatRoughness", texR);
  }

  // obj
  {
    // d
    materials.push_back(cpp::Material("obj"));
    setTexture<float>(materials.back(), "d", texR);

    // Kd
    materials.push_back(cpp::Material("obj"));
    setTexture<vec3f>(materials.back(), "kd", texRGBA);

    // Ks
    materials.push_back(cpp::Material("obj"));
    setTexture<vec3f>(materials.back(), "ks", texRGBA);

    // Ns
    materials.push_back(cpp::Material("obj"));
    materials.back().setParam("ks", vec3f(1.f));
    setTexture<float>(materials.back(), "ns", texR);
  }

  // other
  {
    // metal roughness
    materials.push_back(cpp::Material("metal"));
    materials.back().setParam("eta", vec3f(0.051, 0.043, 0.041));
    materials.back().setParam("k", vec3f(5.3f, 3.6f, 2.3f));
    setTexture<float>(materials.back(), "roughness", texR);

    // luminous emissive
    materials.push_back(cpp::Material("luminous"));
    setTexture(materials.back(), "color", texRGBA, vec3f(1.f, 0.8f, 0.7f));

    // alloy roughness
    materials.push_back(cpp::Material("alloy"));
    materials.back().setParam("color", vec3f(.72f, .45f, .2f));
    materials.back().setParam("edgeColor", vec3f(1.f, 1.f, 1.f));
    setTexture<float>(materials.back(), "roughness", texR);

    // thinglass attenuationColor
    materials.push_back(cpp::Material("thinGlass"));
    setTexture<vec3f>(materials.back(), "attenuationColor", texRGBA);
  }

  return materials;
}

OSP_REGISTER_TESTING_BUILDER(PtMaterialTex, test_pt_material_tex);

} // namespace testing
} // namespace ospray
