// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "Material.ih"
#include "texture/TextureParam.ih"
#include "../bsdfs/Conductor.ih"
#include "../bsdfs/MicrofacetConductor.ih"
#include "math/spectrum.ih"

struct Alloy
{
  PathTraceMaterial super;

  vec3f color; // reflectivity at normal incidence (0 deg)
  TextureParam map_color;
  vec3f edgeColor; // reflectivity at grazing angle (90 deg)
  TextureParam map_edgeColor;
  float roughness; // in [0, 1]; 0==ideally smooth (mirror)
  TextureParam map_roughness;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* varying Alloy_getBSDF(const uniform PathTraceMaterial* uniform super,
                                          uniform ShadingContext* uniform ctx,
                                          const DifferentialGeometry& dg,
                                          const Ray& ray,
                                          const Medium& currentMedium)
{
  const uniform Alloy* uniform self = (const uniform Alloy* uniform)super;
  varying linear3f* uniform frame = LinearSpace3f_create(ctx, frame(dg.Ns));

  const vec3f color = self->color * make_vec3f(dg.color)
    * get3f(self->map_color, dg, make_vec3f(1.f));

  const vec3f edgeColor = self->edgeColor
    * get3f(self->map_edgeColor, dg, make_vec3f(1.f));

  Fresnel *uniform fresnel = FresnelSchlick_create(ctx, color, edgeColor);

  const float roughness = self->roughness
    * get1f(self->map_roughness, dg, 1.f);

  if (roughness == 0.0f)
    return Conductor_create(ctx, frame, fresnel);
  else
    return MicrofacetConductor_create(ctx, frame, fresnel, roughness, 0.f);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_Alloy_set(void* uniform _self
    , const uniform vec3f& color
    , void* uniform map_color
    , const uniform affine2f &xform_color
    , const uniform vec3f& edgeColor
    , void* uniform map_edgeColor
    , const uniform affine2f &xform_edgeColor
    , uniform float roughness
    , void* uniform map_roughness
    , const uniform affine2f &xform_roughness
    )
{
  Alloy* uniform self = (Alloy* uniform)_self;

  self->color = color;
  self->map_color =
    make_TextureParam((uniform Texture2D*)map_color,
        xform_color);

  self->edgeColor = edgeColor;
  self->map_edgeColor =
    make_TextureParam((uniform Texture2D*)map_edgeColor,
        xform_edgeColor);

  self->roughness = roughness;
  self->map_roughness =
    make_TextureParam((uniform Texture2D*)map_roughness,
        xform_roughness);
}

export void* uniform PathTracer_Alloy_create()
{
  Alloy* uniform self = uniform new Alloy;
  PathTraceMaterial_Constructor(&self->super, Alloy_getBSDF);
  uniform affine2f xform = make_AffineSpace2f_identity();
  PathTracer_Alloy_set(self, make_vec3f(0.9f), NULL, xform, make_vec3f(1.f),
      NULL, xform, 0.1f, NULL, xform);
  return self;
}
