// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "CarPaint.h"
// ispc
#include "render/materials/CarPaint_ispc.h"

namespace ospray {
namespace pathtracer {

CarPaint::CarPaint()
{
  getSh()->super.type = ispc::MATERIAL_TYPE_CARPAINT;
  getSh()->super.getBSDF = ispc::CarPaint_getBSDF_addr();
}

std::string CarPaint::toString() const
{
  return "ospray::pathtracer::CarPaint";
}

void CarPaint::commit()
{
  MaterialParam3f baseColor = getMaterialParam3f("baseColor", vec3f(0.8f));
  MaterialParam1f roughness = getMaterialParam1f("roughness", 0.f);
  MaterialParam1f normal = getMaterialParam1f("normal", 1.f);
  bool useFlakeColor = findParam("flakeColor") != nullptr;
  MaterialParam3f flakeColor = getMaterialParam3f("flakeColor", vec3f(0.f));
  MaterialParam1f flakeScale = getMaterialParam1f("flakeScale", 100.f);
  MaterialParam1f flakeDensity = getMaterialParam1f("flakeDensity", 0.f);
  MaterialParam1f flakeSpread = getMaterialParam1f("flakeSpread", 0.3f);
  MaterialParam1f flakeJitter = getMaterialParam1f("flakeJitter", 0.75f);
  MaterialParam1f flakeRoughness = getMaterialParam1f("flakeRoughness", 0.3f);

  MaterialParam1f coat = getMaterialParam1f("coat", 1.f);
  MaterialParam1f coatIor = getMaterialParam1f("coatIor", 1.5f);
  MaterialParam3f coatColor = getMaterialParam3f("coatColor", vec3f(1.f));
  MaterialParam1f coatThickness = getMaterialParam1f("coatThickness", 1.f);
  MaterialParam1f coatRoughness = getMaterialParam1f("coatRoughness", 0.f);
  MaterialParam1f coatNormal = getMaterialParam1f("coatNormal", 1.f);

  MaterialParam3f flipflopColor =
      getMaterialParam3f("flipflopColor", vec3f(1.f));
  MaterialParam1f flipflopFalloff = getMaterialParam1f("flipflopFalloff", 1.f);

  getSh()->baseColor = baseColor.factor;
  getSh()->baseColorMap = baseColor.tex;
  getSh()->roughness = roughness.factor;
  getSh()->roughnessMap = roughness.tex;
  getSh()->normal = normal.factor;
  getSh()->normalMap = normal.tex;
  getSh()->normalRot = normal.rot;
  getSh()->useFlakeColor = useFlakeColor;
  getSh()->flakeColor = flakeColor.factor;
  getSh()->flakeColorMap = flakeColor.tex;
  getSh()->flakeScale = flakeScale.factor;
  getSh()->flakeScaleMap = flakeScale.tex;
  getSh()->flakeDensity = flakeDensity.factor;
  getSh()->flakeDensityMap = flakeDensity.tex;
  getSh()->flakeSpread = flakeSpread.factor;
  getSh()->flakeSpreadMap = flakeSpread.tex;
  getSh()->flakeJitter = flakeJitter.factor;
  getSh()->flakeJitterMap = flakeJitter.tex;
  getSh()->flakeRoughness = flakeRoughness.factor;
  getSh()->flakeRoughnessMap = flakeRoughness.tex;
  getSh()->coat = coat.factor;
  getSh()->coatMap = coat.tex;
  getSh()->coatIor = coatIor.factor;
  getSh()->coatIorMap = coatIor.tex;
  getSh()->coatColor = coatColor.factor;
  getSh()->coatColorMap = coatColor.tex;
  getSh()->coatThickness = coatThickness.factor;
  getSh()->coatThicknessMap = coatThickness.tex;
  getSh()->coatRoughness = coatRoughness.factor;
  getSh()->coatRoughnessMap = coatRoughness.tex;
  getSh()->coatNormal = coatNormal.factor;
  getSh()->coatNormalMap = coatNormal.tex;
  getSh()->coatNormalRot = coatNormal.rot;
  getSh()->flipflopColor = flipflopColor.factor;
  getSh()->flipflopColorMap = flipflopColor.tex;
  getSh()->flipflopFalloff = flipflopFalloff.factor;
  getSh()->flipflopFalloffMap = flipflopFalloff.tex;
}

} // namespace pathtracer
} // namespace ospray
