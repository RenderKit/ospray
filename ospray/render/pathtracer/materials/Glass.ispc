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
#include "../bsdfs/Dielectric.ih"

struct Glass
{
  PathTraceMaterial super;

  Medium mediumInside;
  Medium mediumOutside;

  float absorptionDistance;
  vec3f absorptionColor;
  vec3f absorption_k;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* uniform Glass_getBSDF(const uniform PathTraceMaterial* uniform super,
                                          uniform ShadingContext* uniform ctx,
                                          const DifferentialGeometry& dg,
                                          const Ray& ray,
                                          const Medium& currentMedium)
{
  const uniform Glass* uniform self = (const uniform Glass* uniform)super;

  float eta = eq(currentMedium, self->mediumOutside) ? self->mediumOutside.ior*rcp(self->mediumInside.ior)
                                                     : self->mediumInside.ior*rcp(self->mediumOutside.ior);

  return Dielectric_create(ctx, LinearSpace3f_create(ctx, frame(dg.Ns)), eta);
}

vec3f Glass_getTransparency(const uniform PathTraceMaterial* uniform material,
                            const DifferentialGeometry& dg,
                            const Ray& ray,
                            const Medium& currentMedium)
{
  const uniform Glass* uniform self = (const uniform Glass* uniform)material;

  float eta = eq(currentMedium, self->mediumOutside) ? self->mediumOutside.ior*rcp(self->mediumInside.ior)
                                                     : self->mediumInside.ior*rcp(self->mediumOutside.ior);

  //print("Glass_getTransparency eta = % \n", eta);

  float cosThetaO = max(-dot(ray.dir, dg.Ns), 0.0f);
  return make_vec3f(1.0f-fresnelDielectric(cosThetaO, eta));
}

void Glass_selectNextMedium(const uniform PathTraceMaterial* uniform super,
                            Medium& currentMedium)
{
  const uniform Glass* uniform self = (const uniform Glass* uniform)super;

  if (eq(currentMedium, self->mediumOutside))
    currentMedium = self->mediumInside;
  else
    currentMedium = self->mediumOutside;
}

vec3f Glass_getAbsorption(const uniform PathTraceMaterial* uniform material, float distance)
{
  const uniform Glass* uniform self = (const uniform Glass* uniform)material;

  vec3f r = make_vec3f(1.0f);

  // Beer's law for glass absorption.

  if (self->absorption_k.x != 0)
   r.x = expf(self->absorption_k.x * distance);
  if (self->absorption_k.y != 0)
   r.y = expf(self->absorption_k.y * distance);
  if (self->absorption_k.z != 0)
   r.z = expf(self->absorption_k.z * distance);

  return r;
}

void Glass_Constructor(uniform Glass* uniform self,
                       uniform float iorInside,
                       const uniform vec3f& transmissionInside,
                       uniform float iorOutside,
                       const uniform vec3f& transmissionOutside,
                       uniform float absDistance,
                       const uniform vec3f& absColor)
{
  PathTraceMaterial_Constructor(&self->super, Glass_getBSDF, Glass_getTransparency, Glass_selectNextMedium, Glass_getAbsorption);
  self->mediumInside.ior           = iorInside;
  self->mediumInside.transmission  = transmissionInside;
  self->mediumOutside.ior          = iorOutside;
  self->mediumOutside.transmission = transmissionOutside;
  self->absorptionDistance = absDistance;
  self->absorptionColor = absColor;

  if (absDistance > 0) {
    if (absColor.x > 0)
      self->absorption_k.x = log(absColor.x)/absDistance;
    else 
      self->absorption_k.x = 0;

    if (absColor.y > 0)
      self->absorption_k.y = log(absColor.y)/absDistance;
    else 
      self->absorption_k.y = 0;

    if (absColor.z > 0)
      self->absorption_k.z = log(absColor.z)/absDistance;
    else 
      self->absorption_k.z = 0;
  }
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_Glass_create(uniform float iorInside,
                                             const uniform vec3f& transmissionInside,
                                             uniform float iorOutside,
                                             const uniform vec3f& transmissionOutside,
                                             uniform float absorptionDistance,
                                             const uniform vec3f& absorptionColor)
{
  uniform Glass* uniform self = uniform new uniform Glass;
  Glass_Constructor(self, iorInside, transmissionInside, iorOutside, transmissionOutside,
    absorptionDistance,absorptionColor);
  return self;
}
