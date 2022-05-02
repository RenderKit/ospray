// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.ih"
#include "render/bsdfs/DielectricLayer.ih"
#include "render/bsdfs/Lambert.ih"
#include "render/bsdfs/MicrofacetDielectricLayer.ih"

#include "PlasticShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF *varying Plastic_getBSDF(
    const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  const uniform Plastic *uniform self = (const uniform Plastic *uniform)super;

  varying LinearSpace3f *uniform shadingFrame =
      LinearSpace3f_create(ctx, frame(dg.Ns));
  const vec3f _pigmentColor = self->pigmentColor;
  varying BSDF *varying bsdf = Lambert_create(ctx, shadingFrame, _pigmentColor);

  if (self->roughness == 0.0f) {
    bsdf = DielectricLayer_create(
        ctx, shadingFrame, bsdf, self->eta, make_vec3f(1.0f), 1.0f, 1.0f);
  } else {
    bsdf = MicrofacetDielectricLayer_create(ctx,
        shadingFrame,
        bsdf,
        self->eta,
        make_vec3f(1.0f),
        1.0f,
        self->roughness,
        0.f,
        1.0f);
  }

  return bsdf;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Plastic_getBSDF_addr()
{
  return Plastic_getBSDF;
}
