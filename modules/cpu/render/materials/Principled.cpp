// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Principled.h"
// ispc
#include "render/materials/Principled_ispc.h"

namespace ospray {
namespace pathtracer {

Principled::Principled()
{
  getSh()->super.type = ispc::MATERIAL_TYPE_PRINCIPLED;
  getSh()->super.getBSDF = ispc::Principled_getBSDF_addr();
  getSh()->super.getTransparency = ispc::Principled_getTransparency_addr();
  getSh()->super.selectNextMedium = ispc::Principled_selectNextMedium_addr();
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

  getSh()->baseColor = baseColor.factor;
  getSh()->baseColorMap = baseColor.tex;
  getSh()->edgeColor = edgeColor.factor;
  getSh()->edgeColorMap = edgeColor.tex;
  getSh()->metallic = metallic.factor;
  getSh()->metallicMap = metallic.tex;
  getSh()->diffuse = diffuse.factor;
  getSh()->diffuseMap = diffuse.tex;
  getSh()->specular = specular.factor;
  getSh()->specularMap = specular.tex;
  getSh()->ior = ior.factor;
  getSh()->iorMap = ior.tex;
  getSh()->transmission = transmission.factor;
  getSh()->transmissionMap = transmission.tex;
  getSh()->transmissionColor = transmissionColor.factor;
  getSh()->transmissionColorMap = transmissionColor.tex;
  getSh()->transmissionDepth = transmissionDepth.factor;
  getSh()->transmissionDepthMap = transmissionDepth.tex;
  getSh()->roughness = roughness.factor;
  getSh()->roughnessMap = roughness.tex;
  getSh()->anisotropy = anisotropy.factor;
  getSh()->anisotropyMap = anisotropy.tex;
  getSh()->rotation = rotation.factor;
  getSh()->rotationMap = rotation.tex;
  getSh()->normal = normal.factor;
  getSh()->normalMap = normal.tex;
  getSh()->normalRot = normal.rot;
  getSh()->baseNormal = baseNormal.factor;
  getSh()->baseNormalMap = baseNormal.tex;
  getSh()->baseNormalRot = baseNormal.rot;
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
  getSh()->sheen = sheen.factor;
  getSh()->sheenMap = sheen.tex;
  getSh()->sheenColor = sheenColor.factor;
  getSh()->sheenColorMap = sheenColor.tex;
  getSh()->sheenTint = sheenTint.factor;
  getSh()->sheenTintMap = sheenTint.tex;
  getSh()->sheenRoughness = sheenRoughness.factor;
  getSh()->sheenRoughnessMap = sheenRoughness.tex;
  getSh()->opacity = opacity.factor;
  getSh()->opacityMap = opacity.tex;
  getSh()->thin = thin;
  getSh()->backlight = backlight.factor;
  getSh()->backlightMap = backlight.tex;
  getSh()->thickness = thickness.factor;
  getSh()->thicknessMap = thickness.tex;
  getSh()->outsideMedium.ior = outsideIor >= 1.f ? outsideIor : rcp(outsideIor);
  vec3f otc = vec3f(log(outsideTransmissionColor.x),
      log(outsideTransmissionColor.y),
      log(outsideTransmissionColor.z));
  getSh()->outsideMedium.attenuation = otc / outsideTransmissionDepth;
}

} // namespace pathtracer
} // namespace ospray
