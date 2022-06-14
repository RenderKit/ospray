// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.ih"
#include "render/bsdfs/MultiBSDF.ih"
#include "texture/TextureParam.ih"

#include "MixShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF *varying Mix_getBSDF(const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  const Mix *uniform self = (const Mix *uniform)super;
  varying BSDF *uniform bsdf = MultiBSDF_create(ctx);

  float factor = self->factor * clamp(get1f(self->factorMap, dg, 1.f));

  if (self->mat1)
    MultiBSDF_add(bsdf,
        self->mat1->getBSDF(self->mat1, ctx, dg, ray, currentMedium),
        1.0f - factor,
        1.0f - factor);

  if (self->mat2)
    MultiBSDF_add(bsdf,
        self->mat2->getBSDF(self->mat2, ctx, dg, ray, currentMedium),
        factor,
        factor);

  return bsdf;
}

vec3f Mix_getTransparency(const uniform Material *uniform super,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  const Mix *uniform self = (const Mix *uniform)super;

  vec3f t1 = make_vec3f(0.f);
  if (self->mat1)
    t1 = self->mat1->getTransparency(self->mat1, dg, ray, currentMedium);

  vec3f t2 = make_vec3f(0.f);
  if (self->mat2)
    t2 = self->mat2->getTransparency(self->mat2, dg, ray, currentMedium);

  float factor = self->factor * clamp(get1f(self->factorMap, dg, 1.f));
  return lerp(factor, t1, t2);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Mix_getBSDF_addr()
{
  return Mix_getBSDF;
}

export void *uniform Mix_getTransparency_addr()
{
  return Mix_getTransparency;
}
