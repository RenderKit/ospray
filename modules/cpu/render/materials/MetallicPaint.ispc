// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.ih"
#include "render/bsdfs/DielectricLayer.ih"
#include "render/bsdfs/Lambert.ih"
#include "render/bsdfs/MicrofacetConductor.ih"
#include "render/bsdfs/MultiBSDF.ih"
#include "texture/TextureParam.ih"

#include "MetallicPaintShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF *varying MetallicPaint_getBSDF(
    const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  const uniform MetallicPaint *uniform self =
      (const uniform MetallicPaint *uniform)super;

  varying LinearSpace3f *uniform shadingFrame =
      LinearSpace3f_create(ctx, frame(dg.Ns));
  varying BSDF *uniform bsdf = MultiBSDF_create(ctx);

  const vec3f color = self->baseColor * make_vec3f(dg.color)
      * get3f(self->baseColorMap, dg, make_vec3f(1.f));
  MultiBSDF_add(
      bsdf, Lambert_create(ctx, shadingFrame, color), 1.f, luminance(color));

  if (self->flakeAmount > 0.f) {
    const vec3f r = self->flakeColor;
    const vec3f g = make_vec3f(self->flakeAmount);
    Fresnel *uniform fresnel = FresnelSchlick_create(ctx, r, g);
    MultiBSDF_add(bsdf,
        MicrofacetConductor_create(
            ctx, shadingFrame, fresnel, self->flakeSpread, 0.f),
        1.f,
        luminance(r));
  }

  bsdf = DielectricLayer_create(
      ctx, shadingFrame, bsdf, self->eta, make_vec3f(1.0f), 1.0f, 1.0f);

  return bsdf;
}
///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform MetallicPaint_getBSDF_addr()
{
  return MetallicPaint_getBSDF;
}
