// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../bsdfs/Conductor.ih"
#include "../bsdfs/DielectricLayer.ih"
#include "../bsdfs/Lambert.ih"
#include "../bsdfs/MicrofacetConductor.ih"
#include "../bsdfs/MicrofacetDielectricLayer.ih"
#include "../bsdfs/OrenNayar.ih"
#include "../shaders/Flakes.ih"
#include "Material.ih"
#include "render/pathtracer/bsdfs/MultiBSDF.ih"
#include "texture/TextureParam.ih"

struct CarPaint
{
  PathTraceMaterial super;

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
};

#define EPS 1e-5f

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF *varying CarPaint_getBSDF(
    const uniform PathTraceMaterial *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  const uniform CarPaint *uniform self = (const uniform CarPaint *uniform)super;
  varying BSDF *varying bsdf = NULL;

  varying linear3f *uniform frame = LinearSpace3f_create(ctx,
      makeShadingFrame(dg, self->normalMap, self->normalRot, self->normal));

  const float flakeDensity =
      clamp(self->flakeDensity * get1f(self->flakeDensityMap, dg, 1.f));
  int flakeMask = 0;
  varying linear3f *uniform flakeFrame = NULL;

  // metallic flakes in the clear coat layer
  if (flakeDensity > EPS) {
    const float flakeScale =
        max(self->flakeScale * get1f(self->flakeScaleMap, dg, 1.f), 0.f);
    const float flakeSpread =
        max(self->flakeSpread * get1f(self->flakeSpreadMap, dg, 1.f), 0.f);
    const float flakeJitter =
        clamp(self->flakeJitter * get1f(self->flakeJitterMap, dg, 1.f));

    Flakes flakes;
    flakes.scale = flakeScale;
    flakes.density = flakeDensity;
    flakes.spread = flakeSpread;
    flakes.jitter = flakeJitter;

    const vec3f flakeN = Flakes_eval(flakes, dg.P, flakeMask);
    if (flakeMask) {
      flakeFrame = LinearSpace3f_create(ctx, makeShadingFrame(dg, flakeN));

      Fresnel *uniform flakeFresnel;
      if (self->useFlakeColor) {
        const vec3f r = clamp(
            self->flakeColor * get3f(self->flakeColorMap, dg, make_vec3f(1.f)));
        const vec3f g = make_vec3f(1.f);
        flakeFresnel = FresnelSchlick_create(ctx, r, g);
      } else {
        // flakes are made of aluminum
        const uniform vec3f flakeEta =
            make_vec3f(1.69700277f, 0.879832864f, 0.5301736f);
        const uniform vec3f flakeK =
            make_vec3f(9.30200672f, 6.27604008f, 4.89433956f);
        flakeFresnel = FresnelConductorRGBUniform_create(ctx, flakeEta, flakeK);
      }

      const float flakeRoughness = max(
          self->flakeRoughness * get1f(self->flakeRoughnessMap, dg, 1.f), 0.f);
      if (flakeRoughness < EPS)
        bsdf = Conductor_create(ctx, flakeFrame, flakeFresnel);
      else
        bsdf = MicrofacetConductor_create(
            ctx, flakeFrame, flakeFresnel, flakeRoughness, 0.f);
    }
  }

  // base diffuse layer
  if (!flakeMask) {
    const vec3f baseColor =
        clamp(self->baseColor * get3f(self->baseColorMap, dg, make_vec3f(1.f))
            * make_vec3f(dg.color));
    const float roughness =
        max(self->roughness * get1f(self->roughnessMap, dg, 1.f), 0.f);

    if (roughness < EPS)
      bsdf = Lambert_create(ctx, frame, baseColor);
    else
      bsdf = OrenNayar_create(ctx, frame, baseColor, roughness);
  }

  // clear coat layer
  const float coat = max(self->coat * get1f(self->coatMap, dg, 1.f), 0.f);
  if (coat > EPS) {
    float coatIor = self->coatIor * get1f(self->coatIorMap, dg, 1.f);
    if (coatIor < 1.f)
      coatIor = rcp(coatIor);
    coatIor = clamp(coatIor, 1.f, 3.f); // clamp to common range due to LUTs
    if (abs(coatIor - 1.f) > EPS) {
      // compute the final coat color
      const vec3f coatColor = clamp(
          self->coatColor * get3f(self->coatColorMap, dg, make_vec3f(1.f)));
      vec3f coatFinalColor = coatColor;
      if (flakeMask) {
        const float flipflopFalloff = clamp(
            self->flipflopFalloff * get1f(self->flipflopFalloffMap, dg, 1.f));
        if (flipflopFalloff < 1.f - EPS) {
          // pearlescent flakes
          const vec3f flipflopColor = clamp(self->flipflopColor
              * get3f(self->flipflopColorMap, dg, make_vec3f(1.f)));
          const float cosThetaO = max(-dot(ray.dir, flakeFrame->vz), 0.f);
          const float weight = pow(1.f - cosThetaO,
              rcp(1.f
                  - flipflopFalloff)); // use Schlick for the blending weight
          coatFinalColor = lerp(weight, coatColor, flipflopColor);
        }
      }

      const float coatThickness = max(
          self->coatThickness * get1f(self->coatThicknessMap, dg, 1.f), 0.f);
      const float coatRoughness = max(
          self->coatRoughness * get1f(self->coatRoughnessMap, dg, 1.f), 0.f);
      varying linear3f *uniform coatFrame = LinearSpace3f_create(ctx,
          makeShadingFrame(
              dg, self->coatNormalMap, self->coatNormalRot, self->coatNormal));

      if (coatRoughness < EPS) {
        bsdf = DielectricLayer_create(ctx,
            coatFrame,
            bsdf,
            rcp(coatIor),
            coatFinalColor,
            coatThickness,
            coat);
      } else {
        bsdf = MicrofacetDielectricLayer_create(ctx,
            coatFrame,
            bsdf,
            rcp(coatIor),
            coatFinalColor,
            coatThickness,
            coatRoughness,
            0.f,
            coat);
      }
    }
  }

  return bsdf;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform PathTracer_CarPaint_set(void *uniform _self,
    const uniform vec3f &baseColor,
    const uniform TextureParam &baseColorMap,
    uniform float roughness,
    const uniform TextureParam &roughnessMap,
    uniform float normal,
    const uniform TextureParam &normalMap,
    const uniform linear2f &normalRot,
    const uniform bool useFlakeColor,
    const uniform vec3f &flakeColor,
    const uniform TextureParam &flakeColorMap,
    uniform float flakeScale,
    const uniform TextureParam &flakeScaleMap,
    uniform float flakeDensity,
    const uniform TextureParam &flakeDensityMap,
    uniform float flakeSpread,
    const uniform TextureParam &flakeSpreadMap,
    uniform float flakeJitter,
    const uniform TextureParam &flakeJitterMap,
    uniform float flakeRoughness,
    const uniform TextureParam &flakeRoughnessMap,
    uniform float coat,
    const uniform TextureParam &coatMap,
    uniform float coatIor,
    const uniform TextureParam &coatIorMap,
    const uniform vec3f &coatColor,
    const uniform TextureParam &coatColorMap,
    uniform float coatThickness,
    const uniform TextureParam &coatThicknessMap,
    uniform float coatRoughness,
    const uniform TextureParam &coatRoughnessMap,
    uniform float coatNormal,
    const uniform TextureParam &coatNormalMap,
    const uniform linear2f &coatNormalRot,
    const uniform vec3f &flipflopColor,
    const uniform TextureParam &flipflopColorMap,
    uniform float flipflopFalloff,
    const uniform TextureParam &flipflopFalloffMap)
{
  CarPaint *uniform self = (CarPaint * uniform) _self;

  self->baseColor = baseColor;
  self->baseColorMap = baseColorMap;

  self->roughness = roughness;
  self->roughnessMap = roughnessMap;

  self->normal = normal;
  self->normalMap = normalMap;
  self->normalRot = normalRot;

  self->useFlakeColor = useFlakeColor;
  self->flakeColor = flakeColor;
  self->flakeColorMap = flakeColorMap;

  self->flakeScale = flakeScale;
  self->flakeScaleMap = flakeScaleMap;

  self->flakeDensity = flakeDensity;
  self->flakeDensityMap = flakeDensityMap;

  self->flakeSpread = flakeSpread;
  self->flakeSpreadMap = flakeSpreadMap;

  self->flakeJitter = flakeJitter;
  self->flakeJitterMap = flakeJitterMap;

  self->flakeRoughness = flakeRoughness;
  self->flakeRoughnessMap = flakeRoughnessMap;

  self->coat = coat;
  self->coatMap = coatMap;

  self->coatIor = coatIor;
  self->coatIorMap = coatIorMap;

  self->coatColor = coatColor;
  self->coatColorMap = coatColorMap;

  self->coatThickness = coatThickness;
  self->coatThicknessMap = coatThicknessMap;

  self->coatRoughness = coatRoughness;
  self->coatRoughnessMap = coatRoughnessMap;

  self->coatNormal = coatNormal;
  self->coatNormalMap = coatNormalMap;
  self->coatNormalRot = coatNormalRot;

  self->flipflopColor = flipflopColor;
  self->flipflopColorMap = flipflopColorMap;

  self->flipflopFalloff = flipflopFalloff;
  self->flipflopFalloffMap = flipflopFalloffMap;
}

export void *uniform PathTracer_CarPaint_create()
{
  CarPaint *uniform self = uniform new CarPaint;

  PathTraceMaterial_Constructor(&self->super, CarPaint_getBSDF);

  uniform TextureParam tex = make_TextureParam();
  PathTracer_CarPaint_set(self,
      make_vec3f(0.8f),
      tex, // baseColor
      0.f,
      tex, // roughness
      1.f,
      tex,
      make_LinearSpace2f_identity(), // normal
      0, // useFlakeColor
      make_vec3f(0.f),
      tex, // flakeColor
      0.f,
      tex, // flakeScale
      0.f,
      tex, // flakeDensity
      0.f,
      tex, // flakeSpread
      0.f,
      tex, // flakeJitter
      0.f,
      tex, // flakeRoughness
      1.f,
      tex, // coat
      1.5f,
      tex, // coatIor
      make_vec3f(1.f),
      tex, // coatColor
      1.f,
      tex, // coatThickness
      0.f,
      tex, // coatRoughness
      1.f,
      tex,
      make_LinearSpace2f_identity(), // coatNormal
      make_vec3f(1.f),
      tex, // flipflopColor
      1.f,
      tex // flipflopFalloff
  );

  return self;
}
