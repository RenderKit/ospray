// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "CarPaint.h"
#include "common/Data.h"
#include "math/spectrum.h"
// ispc
#include "render/pathtracer/materials/CarPaint_ispc.h"

namespace ospray {
namespace pathtracer {

CarPaint::CarPaint()
{
  ispcEquivalent = ispc::PathTracer_CarPaint_create();
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

  ispc::PathTracer_CarPaint_set(getIE(),
      (const ispc::vec3f &)baseColor.factor,
      baseColor.tex,
      roughness.factor,
      roughness.tex,
      normal.factor,
      normal.tex,
      (const ispc::LinearSpace2f &)normal.rot,
      useFlakeColor,
      (const ispc::vec3f &)flakeColor.factor,
      flakeColor.tex,
      flakeScale.factor,
      flakeScale.tex,
      flakeDensity.factor,
      flakeDensity.tex,
      flakeSpread.factor,
      flakeSpread.tex,
      flakeJitter.factor,
      flakeJitter.tex,
      flakeRoughness.factor,
      flakeRoughness.tex,

      coat.factor,
      coat.tex,
      coatIor.factor,
      coatIor.tex,
      (const ispc::vec3f &)coatColor.factor,
      coatColor.tex,
      coatThickness.factor,
      coatThickness.tex,
      coatRoughness.factor,
      coatRoughness.tex,
      coatNormal.factor,
      coatNormal.tex,
      (const ispc::LinearSpace2f &)coatNormal.rot,

      (const ispc::vec3f &)flipflopColor.factor,
      flipflopColor.tex,
      flipflopFalloff.factor,
      flipflopFalloff.tex);
}

} // namespace pathtracer
} // namespace ospray
