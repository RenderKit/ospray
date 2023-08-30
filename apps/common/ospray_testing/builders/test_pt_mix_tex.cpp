// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_pt_tex.h"

namespace ospray {
namespace testing {

struct PtMixTex : public PtTex
{
  PtMixTex() = default;
  ~PtMixTex() override = default;

  std::vector<cpp::Material> buildMaterials(
      const cpp::Texture &texR, const cpp::Texture &texRGBA) const override;
};

// Inlined definitions ////////////////////////////////////////////////////

std::vector<cpp::Material> PtMixTex::buildMaterials(
    const cpp::Texture &texR, const cpp::Texture &) const
{
  std::vector<cpp::Material> materials;

  // obj & thinGlass
  {
    cpp::Material obj("obj");
    obj.commit();
    cpp::Material thinGlass("thinGlass");
    thinGlass.commit();
    cpp::Material mix("mix");
    mix.setParam("material1", obj);
    mix.setParam("material2", thinGlass);
    materials.push_back(mix);
    setTexture<float>(materials.back(), "factor", texR);
  }

  // plastic & metal
  {
    cpp::Material plastic("plastic");
    plastic.setParam("pigmentColor", vec3f(.8f, 0.f, 0.f));
    plastic.commit();
    cpp::Material metal("metal");
    metal.commit();
    cpp::Material mix("mix");
    mix.setParam("material1", plastic);
    mix.setParam("material2", metal);
    materials.push_back(mix);
    setTexture<float>(materials.back(), "factor", texR);
  }

  // velvet & metallicPaint
  {
    cpp::Material velvet("velvet");
    cpp::Material metallicPaint("metallicPaint");
    cpp::Material mix("mix");
    velvet.commit();
    metallicPaint.commit();
    mix.setParam("material1", velvet);
    mix.setParam("material2", metallicPaint);
    materials.push_back(mix);
    setTexture<float>(materials.back(), "factor", texR);
  }

  // carPaint & principled
  {
    cpp::Material carPaint("carPaint");
    carPaint.setParam("baseColor", vec3f(.5f));
    carPaint.setParam("flakeColor", vec3f(.8f));
    carPaint.setParam("flakeDensity", 1.f);
    carPaint.setParam("flakeScale", 1000.f);
    carPaint.commit();
    cpp::Material principled("principled");
    principled.setParam("baseColor", vec3f(0.0177f, 0.189f, 0.590f));
    principled.commit();
    cpp::Material mix("mix");
    mix.setParam("material1", carPaint);
    mix.setParam("material2", principled);
    materials.push_back(mix);
    setTexture<float>(materials.back(), "factor", texR);
  }

  return materials;
}

OSP_REGISTER_TESTING_BUILDER(PtMixTex, test_pt_mix_tex);

} // namespace testing
} // namespace ospray
