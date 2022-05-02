// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.ih"
#include "render/bsdfs/RobustThinDielectric.ih"
#include "texture/TextureParam.ih"

#include "ThinGlassShared.h"

///////////////////////////////////////////////////////////////////////////////
// Implementation

static inline const vec3f getAttenuation(
    const ThinGlass *uniform self, const DifferentialGeometry &dg)
{
  vec3f attenuation =
      self->attenuation + logf(make_vec3f(dg.color)) * self->attenuationScale;

  if (valid(self->attenuationColorMap)) {
    vec3f attenuationColor = get3f(self->attenuationColorMap, dg);
    attenuation = attenuation + logf(attenuationColor) * self->attenuationScale;
  }

  return attenuation;
}

const varying BSDF *varying ThinGlass_getBSDF(const Material *uniform super,
    ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  const ThinGlass *uniform self = (const ThinGlass *uniform)super;

  varying linear3f *uniform frame = LinearSpace3f_create(ctx, frame(dg.Ns));
  return RobustThinDielectric_create(
      ctx, frame, self->eta, getAttenuation(self, dg));
}

vec3f ThinGlass_getTransparency(const Material *uniform material,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  const ThinGlass *uniform self = (const ThinGlass *uniform)material;

  float cosThetaO = max(-dot(ray.dir, dg.Ns), 0.0f);
  return RobustThinDielectric_getTransparency(
      cosThetaO, self->eta, getAttenuation(self, dg));
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform ThinGlass_getBSDF_addr()
{
  return ThinGlass_getBSDF;
}

export void *uniform ThinGlass_getTransparency_addr()
{
  return ThinGlass_getTransparency;
}
