// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../bsdfs/Conductor.ih"
#include "../bsdfs/MicrofacetConductor.ih"
#include "Material.ih"
#include "math/spectrum.ih"
#include "texture/TextureParam.ih"

struct Alloy
{
  PathTraceMaterial super;

  vec3f color; // reflectivity at normal incidence (0 deg)
  TextureParam colorMap;
  vec3f edgeColor; // reflectivity at grazing angle (90 deg)
  TextureParam edgeColorMap;
  float roughness; // in [0, 1]; 0==ideally smooth (mirror)
  TextureParam roughnessMap;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF *varying Alloy_getBSDF(
    const uniform PathTraceMaterial *uniform super,
    uniform ShadingContext *uniform ctx,
    const DifferentialGeometry &dg,
    const Ray &ray,
    const Medium &currentMedium)
{
  const uniform Alloy *uniform self = (const uniform Alloy *uniform)super;
  varying linear3f *uniform frame = LinearSpace3f_create(ctx, frame(dg.Ns));

  const vec3f color = self->color * make_vec3f(dg.color)
      * get3f(self->colorMap, dg, make_vec3f(1.f));

  const vec3f edgeColor =
      self->edgeColor * get3f(self->edgeColorMap, dg, make_vec3f(1.f));

  Fresnel *uniform fresnel = FresnelSchlick_create(ctx, color, edgeColor);

  const float roughness = self->roughness * get1f(self->roughnessMap, dg, 1.f);

  if (roughness == 0.0f)
    return Conductor_create(ctx, frame, fresnel);
  else
    return MicrofacetConductor_create(ctx, frame, fresnel, roughness, 0.f);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void *uniform PathTracer_Alloy_set(void *uniform _self,
    const uniform vec3f &color,
    const uniform TextureParam &colorMap,
    const uniform vec3f &edgeColor,
    const uniform TextureParam &edgeColorMap,
    uniform float roughness,
    const uniform TextureParam &roughnessMap)
{
  Alloy *uniform self = (Alloy * uniform) _self;

  self->color = color;
  self->colorMap = colorMap;

  self->edgeColor = edgeColor;
  self->edgeColorMap = edgeColorMap;

  self->roughness = roughness;
  self->roughnessMap = roughnessMap;
}

export void *uniform PathTracer_Alloy_create()
{
  Alloy *uniform self = uniform new Alloy;
  PathTraceMaterial_Constructor(&self->super, Alloy_getBSDF);

  uniform TextureParam tex = make_TextureParam();
  PathTracer_Alloy_set(
      self, make_vec3f(0.9f), tex, make_vec3f(1.f), tex, 0.1f, tex);
  return self;
}
