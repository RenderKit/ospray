// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
  PathTraceMaterial super;
  vec3f reflectance;
  vec3f eta;
  vec3f k;
  float roughness;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* uniform Metal_getBSDF(const uniform PathTraceMaterial* uniform super,
                                          uniform ShadingContext* uniform ctx,
                                          const DifferentialGeometry& dg,
                                          const Ray& ray,
                                          const Medium& currentMedium)
{
  const uniform Metal* uniform self = (const uniform Metal* uniform)super;
  if (self->roughness == 0.0f)
    return Conductor_create(ctx, LinearSpace3f_create(ctx, frame(dg.Ns)), self->reflectance, self->eta, self->k);
  else
    return MicrofacetConductor_create(ctx, LinearSpace3f_create(ctx, frame(dg.Ns)), self->reflectance, self->eta, self->k, self->roughness);
}

void Metal_Constructor(uniform Metal* uniform self,
                       const uniform vec3f& reflectance,
                       const uniform vec3f& eta,
                       const uniform vec3f& k,
                       uniform float roughness)
{
  PathTraceMaterial_Constructor(&self->super, Metal_getBSDF, NULL, NULL, NULL);
  self->reflectance = reflectance;
  self->eta = eta;
  self->k = k;
  self->roughness = roughness;
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
