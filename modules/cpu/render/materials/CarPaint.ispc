// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "CarPaint.ih"

#include "common/Ray.ih"
#include "render/Material.ih"
#include "render/bsdfs/Conductor.ih"
#include "render/bsdfs/DielectricLayer.ih"
#include "render/bsdfs/Lambert.ih"
#include "render/bsdfs/MicrofacetConductor.ih"
#include "render/bsdfs/MicrofacetDielectricLayer.ih"
#include "render/bsdfs/OrenNayar.ih"
#include "render/bsdfs/PassthroughLayer.ih"
#include "render/shaders/Flakes.ih"
#include "texture/TextureParam.ih"
// c++ shared
#include "CarPaintShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

OSPRAY_BEGIN_ISPC_NAMESPACE

struct CarPaint_BSDF
{
  BSDF root;
  MicrofacetDielectricLayer dielectricLayer;
  int flakeMask;
  MicrofacetConductor conductor;
  OrenNayar baseDiffuse;
};

SYCL_EXTERNAL __noinline const varying BSDF *uniform CarPaint_getBSDF(
    const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &,
    const uniform FeatureFlagsHandler &)
{
  const uniform CarPaint *uniform self = (const uniform CarPaint *uniform)super;

  varying linear3f *uniform frame = LinearSpace3f_create(ctx,
      makeShadingFrame(dg, self->normalMap, self->normalRot, self->normal));

  // Allocate memory and initialize material BSDF
  varying CarPaint_BSDF *uniform bsdf = (varying CarPaint_BSDF * uniform)
      ShadingContext_alloc(ctx, sizeof(CarPaint_BSDF));

  const float flakeDensity =
      clamp(self->flakeDensity * get1f(self->flakeDensityMap, dg, 1.f));
  int flakeMask = 0;
  varying linear3f *uniform flakeFrame = NULL;

  varying BSDF *varying substrate = NULL;

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

      if (self->flakeRoughness < EPS)
        Conductor_Constructor((varying Conductor * uniform) & bsdf->conductor,
            flakeFrame,
            flakeFresnel);
      else
        MicrofacetConductor_Constructor(&bsdf->conductor,
            super->microfacetAlbedoTables,
            flakeFrame,
            flakeFresnel,
            max(self->flakeRoughness * get1f(self->flakeRoughnessMap, dg, 1.f),
                0.f),
            0.f);

      substrate = &bsdf->conductor.super;
    }
  }

  // base diffuse layer
  if (!flakeMask) {
    const vec3f baseColor =
        clamp(self->baseColor * get3f(self->baseColorMap, dg, make_vec3f(1.f))
            * make_vec3f(dg.color));

    if (self->roughness < EPS)
      Lambert_Constructor(
          (varying BSDF * uniform) & bsdf->baseDiffuse, frame, baseColor);
    else
      OrenNayar_Constructor(&bsdf->baseDiffuse,
          frame,
          baseColor,
          max(self->roughness * get1f(self->roughnessMap, dg, 1.f), 0.f));

    substrate = &bsdf->baseDiffuse.super;
  }
  bsdf->flakeMask = flakeMask;

  // clear coat layer
  if ((self->coat > EPS)
      && (abs(self->coatIor - 1.f) > EPS || valid(self->coatIorMap))) {
    const float coat = max(self->coat * get1f(self->coatMap, dg, 1.f), 0.f);
    float coatIor = self->coatIor * get1f(self->coatIorMap, dg, 1.f);
    if (coatIor < 1.f)
      coatIor = rcp(coatIor);
    coatIor = clamp(coatIor, 1.f, 3.f); // clamp to common range due to LUTs

    // compute the final coat color
    const vec3f coatColor =
        clamp(self->coatColor * get3f(self->coatColorMap, dg, make_vec3f(1.f)));
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
            rcp(1.f - flipflopFalloff)); // use Schlick for the blending weight
        coatFinalColor = lerp(weight, coatColor, flipflopColor);
      }
    }

    const float coatThickness =
        max(self->coatThickness * get1f(self->coatThicknessMap, dg, 1.f), 0.f);
    varying linear3f *uniform coatFrame = LinearSpace3f_create(ctx,
        makeShadingFrame(
            dg, self->coatNormalMap, self->coatNormalRot, self->coatNormal));

    if (self->coatRoughness < EPS) {
      DielectricLayer_Constructor(
          (varying DielectricLayer * uniform) & bsdf->dielectricLayer,
          coatFrame,
          substrate,
          rcp(coatIor),
          coatFinalColor,
          coatThickness,
          coat);
    } else {
      MicrofacetDielectricLayer_Constructor(&bsdf->dielectricLayer,
          super->microfacetAlbedoTables,
          coatFrame,
          substrate,
          rcp(coatIor),
          coatFinalColor,
          coatThickness,
          max(self->coatRoughness * get1f(self->coatRoughnessMap, dg, 1.f),
              0.f),
          0.f,
          coat);
    }
  } else {
    PassthroughLayer_Constructor(
        (varying PassthroughLayer * uniform) & bsdf->dielectricLayer,
        substrate);
  }

  bsdf->root = bsdf->dielectricLayer.super;
  bsdf->root.bsdfType = BSDF_TYPE_CARPAINT;
  return &bsdf->root;
}

// Conductor BSDF

inline BSDF_EvalRes ConductorBSDF_eval(
    const varying MicrofacetConductor *uniform self,
    const vec3f &wo,
    const vec3f &wi)
{
  if (self->super.bsdfType == BSDF_TYPE_MICROFACET_CONDUCTOR)
    return MicrofacetConductor_eval(self, wo, wi);
  else
    return make_BSDF_EvalRes_zero();
}

inline BSDF_SampleRes ConductorBSDF_sample(
    const varying MicrofacetConductor *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  if (self->super.bsdfType == BSDF_TYPE_MICROFACET_CONDUCTOR)
    return MicrofacetConductor_sample(self, wo, s, ss);
  else
    return Conductor_sample(&self->super, wo, s, ss);
}

// Diffuse BSDF

inline BSDF_EvalRes DiffuseBSDF_eval(
    const varying OrenNayar *uniform self, const vec3f &wo, const vec3f &wi)
{
  if (self->super.bsdfType == BSDF_TYPE_OREN_NAYAR)
    return OrenNayar_eval(&self->super, wo, wi);
  else
    return Lambert_eval(&self->super, wo, wi);
}

inline BSDF_SampleRes DiffuseBSDF_sample(const varying OrenNayar *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  if (self->super.bsdfType == BSDF_TYPE_OREN_NAYAR)
    return OrenNayar_sample(&self->super, wo, s, ss);
  else
    return Lambert_sample(&self->super, wo, s, ss);
}

// Base layer BSDF

__noinline BSDF_EvalRes BaseBSDF_eval(
    const varying CarPaint_BSDF *uniform self, const vec3f &wo, const vec3f &wi)
{
  if (self->flakeMask) {
    return ConductorBSDF_eval(&self->conductor, wo, wi);
  } else {
    return DiffuseBSDF_eval(&self->baseDiffuse, wo, wi);
  }
}

__noinline BSDF_SampleRes BaseBSDF_sample(
    const varying CarPaint_BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  if (self->flakeMask) {
    return ConductorBSDF_sample(&self->conductor, wo, s, ss);
  } else {
    return DiffuseBSDF_sample(&self->baseDiffuse, wo, s, ss);
  }
}

// DielectricLayer BSDF

inline BSDF_EvalRes DielectricLayerBSDF_eval(
    const varying CarPaint_BSDF *uniform self, const vec3f &wo, const vec3f &wi)
{
  BSDFScatteringType scatteringType = (self->flakeMask)
      ? self->conductor.super.scatteringType
      : self->baseDiffuse.super.scatteringType;

  DIELECTRICLAYER_EVAL(
      self->dielectricLayer, scatteringType, self, BaseBSDF_eval);
  return DIELECTRICLAYER_EVAL_GET();
}

inline BSDF_SampleRes DielectricLayerBSDF_sample(
    const varying CarPaint_BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  BSDFScatteringType scatteringType = (self->flakeMask)
      ? self->conductor.super.scatteringType
      : self->baseDiffuse.super.scatteringType;

  DIELECTRICLAYER_SAMPLE(
      self->dielectricLayer, scatteringType, self, BaseBSDF_sample);
  return DIELECTRICLAYER_SAMPLE_GET();
}

// MicrofacetDielectricLayer BSDF

inline BSDF_EvalRes MicrofacetDielectricLayerBSDF_eval(
    const varying CarPaint_BSDF *uniform self, const vec3f &wo, const vec3f &wi)
{
  BSDFScatteringType scatteringType = (self->flakeMask)
      ? self->conductor.super.scatteringType
      : self->baseDiffuse.super.scatteringType;

  MICROFACETDIELECTRICLAYER_EVAL(
      self->dielectricLayer, scatteringType, self, BaseBSDF_eval);
  return MICROFACETDIELECTRICLAYER_EVAL_GET();
}

inline BSDF_SampleRes MicrofacetDielectricLayerBSDF_sample(
    const varying CarPaint_BSDF *uniform self,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  BSDFScatteringType scatteringType = (self->flakeMask)
      ? self->conductor.super.scatteringType
      : self->baseDiffuse.super.scatteringType;

  MICROFACETDIELECTRICLAYER_SAMPLE(self->dielectricLayer,
      scatteringType,
      self,
      BaseBSDF_eval,
      BaseBSDF_sample);
  return MICROFACETDIELECTRICLAYER_SAMPLE_GET();
}

// CarPaint BSDF

SYCL_EXTERNAL __noinline BSDF_EvalRes CarPaint_BSDF_eval(
    const varying BSDF *uniform super, const vec3f &wo, const vec3f &wi)
{
  const varying CarPaint_BSDF *uniform self =
      (const varying CarPaint_BSDF *uniform)super;

  // Skip dielectric layer if no clear coat
  if (self->dielectricLayer.weight < EPS)
    return BaseBSDF_eval(self, wo, wi);

  if (self->dielectricLayer.super.bsdfType
      == BSDF_TYPE_MICROFACET_DIELECTRIC_LAYER)
    return MicrofacetDielectricLayerBSDF_eval(self, wo, wi);
  else
    return DielectricLayerBSDF_eval(self, wo, wi);
}

SYCL_EXTERNAL __noinline BSDF_SampleRes CarPaint_BSDF_sample(
    const varying BSDF *uniform super,
    const vec3f &wo,
    const vec2f &s,
    float ss)
{
  const varying CarPaint_BSDF *uniform self =
      (const varying CarPaint_BSDF *uniform)super;

  // Skip dielectric layer if no clear coat
  if (self->dielectricLayer.weight < EPS)
    return BaseBSDF_sample(self, wo, s, ss);

  if (self->dielectricLayer.super.bsdfType
      == BSDF_TYPE_MICROFACET_DIELECTRIC_LAYER)
    return MicrofacetDielectricLayerBSDF_sample(self, wo, s, ss);
  else
    return DielectricLayerBSDF_sample(self, wo, s, ss);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform CarPaint_getBSDF_addr()
{
  return (void *uniform)CarPaint_getBSDF;
}

OSPRAY_END_ISPC_NAMESPACE
