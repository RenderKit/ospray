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
#include "../bsdfs/Conductor.ih"
#include "../bsdfs/MicrofacetConductor.ih"

struct Metal
{
  PathTraceMaterial base;
  vec3f reflectance;
  vec3f eta;
  vec3f k;
  float roughness;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const uniform BSDF* uniform Metal_getBSDF(const uniform PathTraceMaterial* uniform material,
                                          uniform ShadingContext* uniform ctx,
                                          const DifferentialGeometry& dg,
                                          const Ray& ray,
                                          const Medium& currentMedium)
{
  const uniform Metal* uniform self = (const uniform Metal* uniform)material;
  if (self->roughness == 0.0f)
    return Conductor_create(ctx, LinearSpace3f_create(ctx, frame(dg.Ns)), self->reflectance, self->eta, self->k);
  else
    return MicrofacetConductor_create(ctx, LinearSpace3f_create(ctx, frame(dg.Ns)), self->reflectance, self->eta, self->k, rcp(self->roughness));
}

/*
void Metal__shade(const uniform PathTraceMaterial *uniform _this,
                  const Ray&                  ray,
                  const Medium&               currentMedium,
                  const DifferentialGeometry& dg,
                  uniform CompositedBSDF&     brdfs,
                  vec3f& Ns)
{
  const uniform Metal* uniform this = (const uniform Metal* uniform)_this;
  if (this->roughness == 0.0f)  CompositedBSDF_add(&brdfs,&this->conductor.base);
  else                          CompositedBSDF_add(&brdfs,&this->microfacetMetal.base);
}
*/

void Metal_Constructor(uniform Metal* uniform self,
                       const uniform vec3f& reflectance,
                       const uniform vec3f& eta,
                       const uniform vec3f& k,
                       uniform float roughness)
{
  PathTraceMaterial_Constructor(&self->base, Metal_getBSDF, NULL, NULL);
  self->reflectance = reflectance;
  self->eta = eta;
  self->k = k;
  self->roughness = roughness;
  /*
  Conductor_Constructor(&this->conductor,reflectance, eta, k);
  MicrofacetMetal__Constructor(&this->microfacetMetal,
                               reflectance,
                               make_FresnelConductor(eta,k),
                               make_PowerCosineDistribution(rcp(roughness)));
  */
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_Metal_create(const uniform vec3f& reflectance,
                                             const uniform vec3f& eta,
                                             const uniform vec3f& k,
                                             uniform float roughness)
{
  uniform Metal* uniform self = uniform new uniform Metal;
  Metal_Constructor(self, reflectance, eta, k, roughness);
  return self;
}
