// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "MediumShared.h"
#include "render/MaterialShared.h"
#include "texture/TextureParamShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Principled
{
  Material super;

  vec3f baseColor;
  TextureParam baseColorMap;

  // metallic reflectivity at grazing angle (90 deg) / edge tint
  vec3f edgeColor;
  TextureParam edgeColorMap;

  float metallic;
  TextureParam metallicMap;

  // diffuse weight in [0, 1]
  float diffuse;
  TextureParam diffuseMap;

  // specular weight in [0, 1]
  float specular;
  TextureParam specularMap;

  // index of refraction
  float ior;
  TextureParam iorMap;

  // specular transmission in [0, 1]
  float transmission;
  TextureParam transmissionMap;

  vec3f transmissionColor;
  TextureParam transmissionColorMap;

  float transmissionDepth;
  TextureParam transmissionDepthMap;

  // roughness in [0, 1]; 0 = ideally smooth (mirror)
  float roughness;
  TextureParam roughnessMap;

  // degree of anisotropy in [0, 1]; 0 = isotropic, 1 = maximally anisotropic
  float anisotropy;
  TextureParam anisotropyMap;

  // anisotropic rotation in [0, 1];
  float rotation;
  TextureParam rotationMap;

  // default normal map for all layers
  float normal; // scale
  TextureParam normalMap;
  linear2f normalRot;

  // base normal map (overrides default normal)
  float baseNormal; // scale
  TextureParam baseNormalMap;
  linear2f baseNormalRot;

  // dielectric clear coat weight in [0, 1]
  float coat;
  TextureParam coatMap;

  // dielectric clear coat index of refraction
  float coatIor;
  TextureParam coatIorMap;

  vec3f coatColor;
  TextureParam coatColorMap;

  float coatThickness;
  TextureParam coatThicknessMap;

  float coatRoughness;
  TextureParam coatRoughnessMap;

  // clear coat normal map (overrides default normal)
  float coatNormal; // scale
  TextureParam coatNormalMap;
  linear2f coatNormalRot;

  // sheen weight in [0, 1]
  float sheen;
  TextureParam sheenMap;

  vec3f sheenColor;
  TextureParam sheenColorMap;

  // sheen tint in [0, 1]
  float sheenTint;
  TextureParam sheenTintMap;

  float sheenRoughness;
  TextureParam sheenRoughnessMap;

  // cut-out opacity in [0, 1]
  float opacity;
  TextureParam opacityMap;

  // solid or thin mode flag
  bool thin;

  // diffuse transmission in [0, 2] (thin only)
  float backlight;
  TextureParam backlightMap;

  // thickness (thin only)
  float thickness;
  TextureParam thicknessMap;

  Medium outsideMedium;

#ifdef __cplusplus
  Principled()
      : baseColor(0.8f),
        edgeColor(1.f),
        metallic(0.f),
        diffuse(1.f),
        specular(1.f),
        ior(1.f),
        transmission(0.f),
        transmissionColor(1.f),
        transmissionDepth(1.f),
        roughness(0.f),
        anisotropy(0.f),
        rotation(0.f),
        normal(1.f),
        normalRot(one),
        baseNormal(1.f),
        baseNormalRot(one),
        coat(0.f),
        coatIor(1.5f),
        coatColor(1.f),
        coatThickness(1.f),
        coatRoughness(0.f),
        coatNormal(1.f),
        coatNormalRot(one),
        sheen(0.f),
        sheenColor(1.f),
        sheenTint(0.f),
        sheenRoughness(0.2f),
        opacity(1.f),
        thin(false),
        backlight(0.f),
        thickness(1.f)
  {
    super.type = ispc::MATERIAL_TYPE_PRINCIPLED;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
