// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

struct MetallicPaint
{
  PathTraceMaterial super;
  vec3f shadeColor;
  float glitterSpread;
  float eta;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* uniform MetallicPaint_getBSDF(const uniform PathTraceMaterial* uniform super,
                                                  uniform ShadingContext* uniform ctx,
                                                  const DifferentialGeometry& dg,
                                                  const Ray& ray,
                                                  const Medium& currentMedium)
{
  const uniform MetallicPaint* uniform self = (const uniform MetallicPaint* uniform)super;

  varying LinearSpace3f* uniform shadingFrame = LinearSpace3f_create(ctx, frame(dg.Ns));
  varying BSDF* uniform bsdf = MultiBSDF_create(ctx);

  const vec3f _shadeColor = self->shadeColor;
  MultiBSDF_add(bsdf, 
                Lambert_create(ctx, shadingFrame, _shadeColor), 
                luminance(_shadeColor));

  if (self->glitterSpread > 0.0f)
  {
    const uniform vec3f etaAluminium = RGB_AL_ETA;
    const uniform vec3f kAluminium = RGB_AL_ETA;
    Fresnel *uniform fresnel = FresnelConductorRGBUniform_create(ctx, etaAluminium, kAluminium);
    MultiBSDF_add(bsdf, MicrofacetConductor_create(ctx, shadingFrame, fresnel, 0.8f));
  }

  bsdf = DielectricLayer_create(ctx, shadingFrame, bsdf, self->eta, make_vec3f(1.0f), 1.0f);

  return bsdf;
}

export void* uniform PathTracer_MetallicPaint_create()
{
  uniform MetallicPaint* uniform self = uniform new uniform MetallicPaint;
  return self;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_MetallicPaint_set(void* uniform _self, const uniform vec3f& shadeColor,
                                                     const uniform float glitterSpread,
                                                     const uniform float ior)
{
  uniform MetallicPaint* uniform self = (uniform MetallicPaint* uniform)_self;
  self->shadeColor = shadeColor;
  self->glitterSpread = glitterSpread;
  self->eta = rcp(ior);
  PathTraceMaterial_Constructor(&self->super, MetallicPaint_getBSDF);
}
