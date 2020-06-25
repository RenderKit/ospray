// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Principled.h"
#include "common/Data.h"
#include "math/spectrum.h"
// ispc
#include "render/pathtracer/materials/Principled_ispc.h"

namespace ospray {
namespace pathtracer {

Principled::Principled()
{
  ispcEquivalent = ispc::PathTracer_Principled_create();
}

std::string Principled::toString() const
{
  return "ospray::pathtracer::Principled";
}

void Principled::commit()
{
  MaterialParam3f baseColor = getMaterialParam3f("baseColor", vec3f(0.8f));
  MaterialParam3f edgeColor = getMaterialParam3f("edgeColor", vec3f(1.f));
  MaterialParam1f metallic = getMaterialParam1f("metallic", 0.f);
  MaterialParam1f diffuse = getMaterialParam1f("diffuse", 1.f);
  MaterialParam1f specular = getMaterialParam1f("specular", 1.f);
  MaterialParam1f ior = getMaterialParam1f("ior", 1.f);
  MaterialParam1f transmission = getMaterialParam1f("transmission", 0.f);
  MaterialParam3f transmissionColor =
      getMaterialParam3f("transmissionColor", vec3f(1.f));
  MaterialParam1f transmissionDepth =
      getMaterialParam1f("transmissionDepth", 1.f);
  MaterialParam1f roughness = getMaterialParam1f("roughness", 0.f);
  MaterialParam1f anisotropy = getMaterialParam1f("anisotropy", 0.f);
  MaterialParam1f rotation = getMaterialParam1f("rotation", 0.f);
  MaterialParam1f normal = getMaterialParam1f("normal", 1.f);
  MaterialParam1f baseNormal = getMaterialParam1f("baseNormal", 1.f);

  MaterialParam1f coat = getMaterialParam1f("coat", 0.f);
  MaterialParam1f coatIor = getMaterialParam1f("coatIor", 1.5f);
  MaterialParam3f coatColor = getMaterialParam3f("coatColor", vec3f(1.f));
  MaterialParam1f coatThickness = getMaterialParam1f("coatThickness", 1.f);
  MaterialParam1f coatRoughness = getMaterialParam1f("coatRoughness", 0.f);
  MaterialParam1f coatNormal = getMaterialParam1f("coatNormal", 1.f);

  MaterialParam1f sheen = getMaterialParam1f("sheen", 0.f);
  MaterialParam3f sheenColor = getMaterialParam3f("sheenColor", vec3f(1.f));
  MaterialParam1f sheenTint = getMaterialParam1f("sheenTint", 0.f);
  MaterialParam1f sheenRoughness = getMaterialParam1f("sheenRoughness", 0.2f);

  MaterialParam1f opacity = getMaterialParam1f("opacity", 1.f);

  bool thin = getParam<bool>("thin", false);
  MaterialParam1f backlight = getMaterialParam1f("backlight", 0.f);
  MaterialParam1f thickness = getMaterialParam1f("thickness", 1.f);

  float outsideIor = getParam<float>("outsideIor", 1.f);
  vec3f outsideTransmissionColor =
      getParam<vec3f>("outsideTransmissionColor", vec3f(1.f));
  float outsideTransmissionDepth =
      getParam<float>("outsideTransmissionDepth", 1.f);

  ispc::PathTracer_Principled_set(getIE(),
      (const ispc::vec3f &)baseColor.factor,
      baseColor.tex,
      (const ispc::vec3f &)edgeColor.factor,
      edgeColor.tex,
      metallic.factor,
      metallic.tex,
      diffuse.factor,
      diffuse.tex,
      specular.factor,
      specular.tex,
      ior.factor,
      ior.tex,
      transmission.factor,
      transmission.tex,
      (const ispc::vec3f &)transmissionColor.factor,
      transmissionColor.tex,
      transmissionDepth.factor,
      transmissionDepth.tex,
      roughness.factor,
      roughness.tex,
      anisotropy.factor,
      anisotropy.tex,
      rotation.factor,
      rotation.tex,
      normal.factor,
      normal.tex,
      (const ispc::LinearSpace2f &)normal.rot,
      baseNormal.factor,
      baseNormal.tex,
      (const ispc::LinearSpace2f &)baseNormal.rot,

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

      sheen.factor,
      sheen.tex,
      (const ispc::vec3f &)sheenColor.factor,
      sheenColor.tex,
      sheenTint.factor,
      sheenTint.tex,
      sheenRoughness.factor,
      sheenRoughness.tex,

      opacity.factor,
      opacity.tex,

      thin,
      backlight.factor,
      backlight.tex,
      thickness.factor,
      thickness.tex,

      outsideIor,
      (const ispc::vec3f &)outsideTransmissionColor,
      outsideTransmissionDepth);
}

} // namespace pathtracer
} // namespace ospray
