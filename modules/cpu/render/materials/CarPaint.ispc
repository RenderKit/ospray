// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.ih"
#include "render/bsdfs/Conductor.ih"
#include "render/bsdfs/DielectricLayer.ih"
#include "render/bsdfs/Lambert.ih"
#include "render/bsdfs/MicrofacetConductor.ih"
#include "render/bsdfs/MicrofacetDielectricLayer.ih"
#include "render/bsdfs/MultiBSDF.ih"
#include "render/bsdfs/OrenNayar.ih"
#include "render/shaders/Flakes.ih"
#include "texture/TextureParam.ih"

#include "CarPaintShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF *varying CarPaint_getBSDF(
    const uniform Material *uniform super,
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

export void *uniform CarPaint_getBSDF_addr()
{
  return CarPaint_getBSDF;
}
