// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.ih"
#include "render/bsdfs/Minneart.ih"
#include "render/bsdfs/MultiBSDF.ih"
#include "render/bsdfs/Velvety.ih"

#include "VelvetShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF *varying Velvet_getBSDF(
    const uniform Material *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  const uniform Velvet *uniform self = (const uniform Velvet *uniform)super;

  varying LinearSpace3f *uniform shadingFrame =
      LinearSpace3f_create(ctx, frame(dg.Ns));
  varying BSDF *uniform bsdf = MultiBSDF_create(ctx);

  MultiBSDF_add(bsdf,
      Minneart_create(
          ctx, shadingFrame, self->reflectance, self->backScattering),
      1.f,
      luminance(self->reflectance));
  MultiBSDF_add(bsdf,
      Velvety_create(ctx,
          shadingFrame,
          self->horizonScatteringColor,
          self->horizonScatteringFallOff),
      1.f,
      luminance(self->horizonScatteringColor));

  return bsdf;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform Velvet_getBSDF_addr()
{
  return Velvet_getBSDF;
}
