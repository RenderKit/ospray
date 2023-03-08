// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/MaterialShared.h"
#include "texture/TextureParamShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct CarPaint
{
  Material super;

  vec3f baseColor;
  TextureParam baseColorMap;

  float roughness;
  TextureParam roughnessMap;

  float normal; // scale
  TextureParam normalMap;
  linear2f normalRot;

  bool useFlakeColor; // if disabled, flakes are Aluminium
  vec3f flakeColor;
  TextureParam flakeColorMap;

  float flakeScale;
  TextureParam flakeScaleMap;

  float flakeDensity;
  TextureParam flakeDensityMap;

  float flakeSpread;
  TextureParam flakeSpreadMap;

  float flakeJitter;
  TextureParam flakeJitterMap;

  float flakeRoughness;
  TextureParam flakeRoughnessMap;

  // dielectric clear coat reflectivity in [0, 1]
  float coat;
  TextureParam coatMap;

  // dielectric clear coat index of refraction in [1, 3]
  float coatIor;
  TextureParam coatIorMap;

  float coatThickness;
  TextureParam coatThicknessMap;

  float coatRoughness;
  TextureParam coatRoughnessMap;

  float coatNormal; // scale
  TextureParam coatNormalMap;
  linear2f coatNormalRot;

  vec3f coatColor;
  TextureParam coatColorMap;

  vec3f flipflopColor;
  TextureParam flipflopColorMap;

  float flipflopFalloff;
  TextureParam flipflopFalloffMap;

#ifdef __cplusplus
  CarPaint()
      : baseColor(0.8f),
        roughness(0.f),
        normal(1.f),
        normalRot(one),
        useFlakeColor(false),
        flakeColor(0.f),
        flakeScale(0.f),
        flakeDensity(0.f),
        flakeSpread(0.f),
        flakeJitter(0.f),
        flakeRoughness(0.f),
        coat(1.f),
        coatIor(1.5f),
        coatThickness(1.f),
        coatRoughness(0.f),
        coatNormal(1.f),
        coatNormalRot(one),
        coatColor(1.f),
        flipflopColor(1.f),
        flipflopFalloff(1.f)
  {
    super.type = ispc::MATERIAL_TYPE_CARPAINT;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
