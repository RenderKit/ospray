// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Material.ih"
#include "render/pathtracer/bsdfs/RobustDielectric.ih"
#include "texture/TextureParam.ih"

struct Glass
{
  PathTraceMaterial super;

  Medium mediumInside;
  Medium mediumOutside;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF *varying Glass_getBSDF(
    const uniform PathTraceMaterial *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  uniform const Glass *uniform self = (uniform const Glass *uniform)super;

  float eta = eq(currentMedium, self->mediumOutside)
      ? self->mediumOutside.ior * rcp(self->mediumInside.ior)
      : self->mediumInside.ior * rcp(self->mediumOutside.ior);

  varying BSDF *uniform bsdf = RobustDielectric_create(ctx, dg, eta);
  return bsdf;
}

vec3f Glass_getTransparency(const uniform PathTraceMaterial *uniform material,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  const uniform Glass *uniform self = (const uniform Glass *uniform)material;

  float eta = eq(currentMedium, self->mediumOutside)
      ? self->mediumOutside.ior * rcp(self->mediumInside.ior)
      : self->mediumInside.ior * rcp(self->mediumOutside.ior);

  float cosThetaO = max(-dot(ray.dir, dg.Ns), 0.0f);
  return make_vec3f(1.0f - fresnelDielectric(cosThetaO, eta));
}

void Glass_selectNextMedium(const uniform PathTraceMaterial *uniform super,
    const DifferentialGeometry &dg,
    Medium &currentMedium)
{
  const uniform Glass *uniform self = (const uniform Glass *uniform)super;

  if (eq(currentMedium, self->mediumOutside))
    currentMedium = self->mediumInside;
  else
    currentMedium = self->mediumOutside;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void PathTracer_Glass_set(void *uniform _self,
    const uniform float iorInside,
    const uniform vec3f &attenuationColorInside,
    const uniform float outsideIor,
    const uniform vec3f &attenuationColorOutside,
    const uniform float attenuationDistance)
{
  uniform Glass *uniform self = (uniform Glass * uniform) _self;

  self->mediumInside.ior = iorInside;
  self->mediumInside.attenuation =
      logf(attenuationColorInside) / attenuationDistance;
  self->mediumOutside.ior = outsideIor;
  self->mediumOutside.attenuation =
      logf(attenuationColorOutside) / attenuationDistance;
}

export void *uniform PathTracer_Glass_create()
{
  uniform Glass *uniform self = uniform new uniform Glass;
  PathTraceMaterial_Constructor(&self->super,
      Glass_getBSDF,
      Glass_getTransparency,
      Glass_selectNextMedium);

  return self;
}
