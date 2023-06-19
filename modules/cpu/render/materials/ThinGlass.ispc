// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/Ray.ih"
#include "render/Material.ih"
#include "render/bsdfs/RobustThinDielectric.ih"
#include "texture/TextureParam.ih"

#include "ThinGlass.ih"

///////////////////////////////////////////////////////////////////////////////
// Implementation
OSPRAY_BEGIN_ISPC_NAMESPACE

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

SYCL_EXTERNAL const varying BSDF *varying ThinGlass_getBSDF(
    const Material *uniform super,
    ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &,
    const Medium &)
{
  const ThinGlass *uniform self = (const ThinGlass *uniform)super;

  varying linear3f *uniform shadingframe =
      LinearSpace3f_create(ctx, frame(dg.Ns));
  return RobustThinDielectric_create(
      ctx, shadingframe, self->eta, getAttenuation(self, dg));
}

SYCL_EXTERNAL vec3f ThinGlass_getTransparency(const Material *uniform material,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &)
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
  return (void *uniform)ThinGlass_getBSDF;
}

export void *uniform ThinGlass_getTransparency_addr()
{
  return (void *uniform)ThinGlass_getTransparency;
}

OSPRAY_END_ISPC_NAMESPACE
