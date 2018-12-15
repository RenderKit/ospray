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
#include "../bsdfs/MultiBSDF.ih"
#include "../bsdfs/Lambert.ih"
#include "../bsdfs/MicrofacetConductor.ih"
#include "../bsdfs/DielectricLayer.ih"
#include "texture/TextureParam.ih"

struct MetallicPaint
{
  PathTraceMaterial super;
  vec3f baseColor;
  TextureParam map_baseColor;
  float flakeAmount;
  vec3f flakeColor;
  float flakeSpread;
  float eta;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* varying MetallicPaint_getBSDF(const uniform PathTraceMaterial* uniform super,
                                                  uniform ShadingContext* uniform ctx,
                                                  const DifferentialGeometry& dg,
                                                  const Ray& ray,
                                                  const Medium& currentMedium)
{
  const uniform MetallicPaint* uniform self = (const uniform MetallicPaint* uniform)super;

  varying LinearSpace3f* uniform shadingFrame = LinearSpace3f_create(ctx, frame(dg.Ns));
  varying BSDF* uniform bsdf = MultiBSDF_create(ctx);

  const vec3f color = self->baseColor * make_vec3f(dg.color)
    * get3f(self->map_baseColor, dg, make_vec3f(1.f));
  MultiBSDF_add(bsdf,
                Lambert_create(ctx, shadingFrame, color),
                1.f,
                luminance(color));

  if (self->flakeAmount > 0.f) {
    const vec3f r = self->flakeColor;
    const vec3f g = make_vec3f(self->flakeAmount);
    Fresnel *uniform fresnel = FresnelSchlick_create(ctx, r, g);
    MultiBSDF_add(bsdf,
      MicrofacetConductor_create(ctx, shadingFrame, fresnel, self->flakeSpread, 0.f), 1.f, luminance(r));
  }

  bsdf = DielectricLayer_create(ctx, shadingFrame, bsdf, self->eta, make_vec3f(1.0f), 1.0f, 1.0f);

  return bsdf;
}
///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_MetallicPaint_set(void* uniform _self
    , const uniform vec3f& baseColor
    , void* uniform map_color
    , const uniform affine2f &xform_color
    , const uniform float flakeAmount
    , const uniform vec3f& flakeColor
    , const uniform float flakeSpread
    , const uniform float ior
    )
{
  MetallicPaint* uniform self = (MetallicPaint* uniform)_self;
  self->flakeAmount = flakeAmount;
  self->baseColor = baseColor * (1.f - flakeAmount);
  self->map_baseColor =
    make_TextureParam((uniform Texture2D*)map_color, xform_color);
  self->flakeColor = flakeColor * flakeAmount;
  self->flakeSpread = flakeSpread;
  self->eta = rcp(ior);
}

export void* uniform PathTracer_MetallicPaint_create()
{
  uniform MetallicPaint* uniform self = uniform new uniform MetallicPaint;
  PathTraceMaterial_Constructor(&self->super, MetallicPaint_getBSDF);
  const uniform vec3f flakeColor = RGB_AL_COLOR;
  PathTracer_MetallicPaint_set(self
    , make_vec3f(0.5)
    , NULL
    , make_AffineSpace2f_identity()
    , 0.5f
    , flakeColor
    , 0.5f
    , 1.6f
    );
  return self;
}
