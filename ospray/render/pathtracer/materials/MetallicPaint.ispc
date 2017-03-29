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
  vec3f glitterColor;
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

  MultiBSDF_add(bsdf, Lambert_create(ctx, shadingFrame, self->shadeColor), luminance(self->shadeColor));

  if (self->glitterSpread != 0.0f & ne(self->glitterColor, make_vec3f(0.0f)))
  {
    const vec3f etaAluminium = make_vec3f(1.69700277f, 0.879832864f, 0.5301736f);
    const vec3f kAluminium   = make_vec3f(9.30200672f, 6.27604008f, 4.89433956f);
    MultiBSDF_add(bsdf, MicrofacetConductor_create(ctx, shadingFrame, self->glitterColor, etaAluminium, kAluminium, self->glitterSpread), luminance(self->glitterColor));
  }

  bsdf = DielectricLayer_create(ctx, shadingFrame, bsdf, self->eta, make_vec3f(1.0f), 1.0f);

  return bsdf;
}

inline void MetallicPaint_Constructor(uniform MetallicPaint* uniform self,
                                      const uniform vec3f& shadeColor,
                                      const uniform vec3f& glitterColor,
                                      const uniform float glitterSpread,
                                      const uniform float ior)
{
  PathTraceMaterial_Constructor(&self->super, MetallicPaint_getBSDF);
  self->shadeColor = shadeColor;
  self->glitterColor = glitterColor;
  self->glitterSpread = glitterSpread;
  self->eta = rcp(ior);
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_MetallicPaint_create(const uniform vec3f& shadeColor,
                                                     const uniform vec3f& glitterColor,
                                                     const uniform float glitterSpread,
                                                     const uniform float ior)
{
  uniform MetallicPaint* uniform self = uniform new uniform MetallicPaint;
  MetallicPaint_Constructor(self,shadeColor,glitterColor,glitterSpread,ior);
  return self;
}
