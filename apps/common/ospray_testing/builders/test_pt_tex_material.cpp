// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_pt_tex.h"

namespace ospray {
namespace testing {

struct PtTexMaterial : public PtTex
{
  PtTexMaterial() = default;
  ~PtTexMaterial() override = default;

  std::vector<cpp::Material> buildMaterials(
      const cpp::Texture &texR, const cpp::Texture &texRGBA) const override;
};

// Inlined definitions ////////////////////////////////////////////////////

std::vector<cpp::Material> PtTexMaterial::buildMaterials(
    const cpp::Texture &texR, const cpp::Texture &texRGBA) const
{
  std::vector<cpp::Material> materials;

  // principled metal
  {
    // metallic
    materials.push_back(cpp::Material("principled"));
    materials.back().setParam("baseColor", vec3f(0.0261f, 0.195f, 0.870f));
    setTexture<float>(materials.back(), "metallic", texR);

    // roughness
    materials.push_back(cpp::Material("principled"));
    materials.back().setParam("baseColor", vec3f(0.0261f, 0.195f, 0.870f));
    materials.back().setParam("metallic", 1.f);
    setTexture<float>(materials.back(), "roughness", texR);

    // anisotropy
    materials.push_back(cpp::Material("principled"));
    materials.back().setParam("baseColor", vec3f(0.0261f, 0.195f, 0.870f));
    materials.back().setParam("metallic", 1.f);
    materials.back().setParam("roughness", .5f);
    setTexture<float>(materials.back(), "anisotropy", texR);

    // coat
    materials.push_back(cpp::Material("principled"));
    materials.back().setParam("baseColor", vec3f(0.860f, 0.0258f, 0.0536f));
    materials.back().setParam("metallic", 1.f);
    materials.back().setParam("roughness", .5f);
    setTexture<float>(materials.back(), "coat", texR);

    // coatRoughness
    materials.push_back(cpp::Material("principled"));
    materials.back().setParam("baseColor", vec3f(0.860f, 0.0258f, 0.0536f));
    materials.back().setParam("metallic", 1.f);
    materials.back().setParam("roughness", .5f);
    materials.back().setParam("coat", 1.f);
    setTexture<float>(materials.back(), "coatRoughness", texR);
  }

  // principled plastic
  {
    // roughness
    materials.push_back(cpp::Material("principled"));
    materials.back().setParam("baseColor", vec3f(0.770f, 0.00f, 0.00f));
    setTexture<float>(materials.back(), "roughness", texR);

    // specular
    materials.push_back(cpp::Material("principled"));
    materials.back().setParam("baseColor", vec3f(0.770f, 0.00f, 0.00f));
    materials.back().setParam("ior", 1.45f);
    setTexture<float>(materials.back(), "specular", texR);

    // sheen
    materials.push_back(cpp::Material("principled"));
    materials.back().setParam("baseColor", vec3f(0.950f, 0.494f, 0.0380f));
    materials.back().setParam("sheenRoughness", .5f);
    setTexture<float>(materials.back(), "sheen", texR);

    // sheenTint
    materials.push_back(cpp::Material("principled"));
    materials.back().setParam("baseColor", vec3f(0.950f, 0.494f, 0.0380f));
    materials.back().setParam("sheen", 1.f);
    materials.back().setParam("sheenRoughness", .5f);
    setTexture<float>(materials.back(), "sheenTint", texR);

    // sheenRoughness
    materials.push_back(cpp::Material("principled"));
    materials.back().setParam("baseColor", vec3f(0.950f, 0.494f, 0.0380f));
    materials.back().setParam("sheen", 1.f);
    setTexture<float>(materials.back(), "sheenRoughness", texR);
  }

  // principled glass
  {
    // transmission
    materials.push_back(cpp::Material("principled"));
    materials.back().setParam("baseColor", vec3f(0.0400f, 0.680f, 1.00f));
    materials.back().setParam(
        "transmissionColor", vec3f(0.0400f, 0.680f, 1.00f));
    materials.back().setParam("ior", 1.1f);
    setTexture<float>(materials.back(), "transmission", texR);

    // roughness
    materials.push_back(cpp::Material("principled"));
    materials.back().setParam("baseColor", vec3f(0.0400f, 0.680f, 1.00f));
    materials.back().setParam(
        "transmissionColor", vec3f(0.0400f, 0.680f, 1.00f));
    materials.back().setParam("transmission", 1.f);
    materials.back().setParam("ior", 1.5f);
    setTexture<float>(materials.back(), "roughness", texR);

    // thickness
    materials.push_back(cpp::Material("principled"));
    materials.back().setParam("baseColor", vec3f(0.0400f, 0.680f, 1.00f));
    materials.back().setParam(
        "transmissionColor", vec3f(0.0400f, 0.680f, 1.00f));
    materials.back().setParam("transmission", 1.f);
    materials.back().setParam("ior", 1.5f);
    materials.back().setParam("thin", true);
    setTexture<float>(materials.back(), "thickness", texR);

    // thin roughness
    materials.push_back(cpp::Material("principled"));
    materials.back().setParam("baseColor", vec3f(0.0400f, 0.680f, 1.00f));
    materials.back().setParam(
        "transmissionColor", vec3f(0.0400f, 0.680f, 1.00f));
    materials.back().setParam("transmission", 1.f);
    materials.back().setParam("ior", 1.5f);
    materials.back().setParam("thin", true);
    setTexture<float>(materials.back(), "roughness", texR);

    // thin backlight
    materials.push_back(cpp::Material("principled"));
    materials.back().setParam("baseColor", vec3f(0.0400f, 0.680f, 1.00f));
    materials.back().setParam(
        "transmissionColor", vec3f(0.0400f, 0.680f, 1.00f));
    materials.back().setParam("transmission", .3f);
    materials.back().setParam("ior", 1.5f);
    materials.back().setParam("thin", true);
    setTexture<float>(materials.back(), "backlight", texR, 2.f);
  }

  // carPaint
  {
    // roughness
    materials.push_back(cpp::Material("carPaint"));
    materials.back().setParam("baseColor", vec3f(0.0177f, 0.189f, 0.590f));
    setTexture<float>(materials.back(), "roughness", texR);

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

    // coat
    materials.push_back(cpp::Material("carPaint"));
    materials.back().setParam("baseColor", vec3f(0.0177f, 0.189f, 0.590f));
    setTexture<float>(materials.back(), "coat", texR);

    // coatRoughness
    materials.push_back(cpp::Material("carPaint"));
    materials.back().setParam("baseColor", vec3f(0.0177f, 0.189f, 0.590f));
    setTexture<float>(materials.back(), "coatRoughness", texR);
  }

  // obj & metal
  {
    // kd
    materials.push_back(cpp::Material("obj"));
    setTexture<vec3f>(materials.back(), "kd", texRGBA);

    // ks
    materials.push_back(cpp::Material("obj"));
    setTexture<vec3f>(materials.back(), "ks", texRGBA);

    // ns
    materials.push_back(cpp::Material("obj"));
    materials.back().setParam("ks", vec3f(1.f));
    setTexture<float>(materials.back(), "ns", texR);

    // d
    materials.push_back(cpp::Material("obj"));
    setTexture<float>(materials.back(), "d", texR);

    // roughness
    materials.push_back(cpp::Material("metal"));
    materials.back().setParam("eta", vec3f(0.051, 0.043, 0.041));
    materials.back().setParam("k", vec3f(5.3f, 3.6f, 2.3f));
    setTexture<float>(materials.back(), "roughness", texR);
  }

  return materials;
}

OSP_REGISTER_TESTING_BUILDER(PtTexMaterial, test_pt_tex_material);

} // namespace testing
} // namespace ospray
