// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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
#include "../bsdfs/DielectricReflection.ih"
#include "../bsdfs/DielectricLayerLambert.ih"
#include "../bsdfs/DielectricLayerMicrofacetMetal.ih"

struct MetallicPaint
{
  PathTraceMaterial base;
  DielectricReflection              reflection;
  DielectricLayerLambert         paint;
  DielectricLayerMicrofacetMetal    glitter;
  bool                              hasGlitter;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

void MetallicPaint__shade(const uniform PathTraceMaterial *uniform _this,
                          const Ray&                  ray,
                          const Medium&               currentMedium,
                          const DifferentialGeometry& dg,
                          uniform CompositedBSDF&     brdfs,
                          vec3f& Ns)
{
  uniform const MetallicPaint *uniform this = (uniform const MetallicPaint *uniform)_this;
  CompositedBSDF_add(&brdfs,&this->reflection.base);
  CompositedBSDF_add(&brdfs,&this->paint.base);
  if (this->hasGlitter) CompositedBSDF_add(&brdfs,&this->glitter.base);
}

inline void MetallicPaint__Constructor(uniform MetallicPaint *uniform this,
                                       const uniform vec3f& shadeColor,
                                       const uniform vec3f& glitterColor,
                                       const uniform float glitterSpread,
                                       const uniform float eta)
{
  PathTraceMaterial__Constructor(&this->base, MetallicPaint__shade, NULL);
  DielectricReflection__Constructor(&this->reflection, 1.0f, eta);
  DielectricLayerLambert_Constructor(&this->paint, make_vec3f(1.0f), 1.0f, eta, make_Lambert(shadeColor));

  uniform vec3f etaAluminium = make_vec3f(0.62f,0.62f,0.62f);
  uniform vec3f kAluminium   = make_vec3f(4.8f,4.8f,4.8f);
  DielectricLayerMicrofacetMetal__Constructor(&this->glitter,
                                              make_vec3f(1.f), 1.f, eta,
                                              make_MicrofacetMetal(glitterColor,
                                                                   make_FresnelConductor(etaAluminium,kAluminium),
                                                                   make_PowerCosineDistribution(rcp(glitterSpread))));
  this->hasGlitter = (glitterSpread != 0.0f & ne(glitterColor,make_vec3f(0.0f)));
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_MetallicPaint_create(const uniform vec3f& shadeColor,
                                                     const uniform vec3f& glitterColor,
                                                     const uniform float glitterSpread,
                                                     const uniform float eta)
{
  uniform MetallicPaint *uniform this = uniform new uniform MetallicPaint;
  MetallicPaint__Constructor(this,shadeColor,glitterColor,glitterSpread,eta);
  return this;
}
