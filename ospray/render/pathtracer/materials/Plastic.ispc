// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../bsdfs/DielectricLayer.ih"
#include "../bsdfs/Lambert.ih"
#include "../bsdfs/MicrofacetDielectricLayer.ih"
#include "Material.ih"

struct Plastic
{
  PathTraceMaterial super;
  vec3f pigmentColor;
  float eta;
  float roughness;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF *varying Plastic_getBSDF(
    const uniform PathTraceMaterial *uniform super,
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

void Plastic_Constructor(uniform Plastic *uniform self,
    const uniform vec3f &pigmentColor,
    uniform float ior,
    uniform float roughness)
{
  PathTraceMaterial_Constructor(&self->super, Plastic_getBSDF);
  self->pigmentColor = pigmentColor;
  self->eta = rcp(ior);
  self->roughness = roughness;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform PathTracer_Plastic_create()
{
  uniform Plastic *uniform self = uniform new uniform Plastic;
  return self;
}

export void *uniform PathTracer_Plastic_set(void *uniform _self,
    const uniform vec3f &pigmentColor,
    uniform float ior,
    uniform float roughness)
{
  uniform Plastic *uniform self = (uniform Plastic * uniform) _self;
  Plastic_Constructor(self, pigmentColor, ior, roughness);
  return self;
}
