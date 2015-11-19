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
#include "../bsdfs/Dielectric.ih"

struct Glass
{
  PathTraceMaterial base;

  Medium mediumInside;
  Medium mediumOutside;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const uniform BSDF* uniform Glass_getBSDF(const uniform PathTraceMaterial* uniform material,
                                          uniform ShadingContext* uniform ctx,
                                          const DifferentialGeometry& dg,
                                          const Ray& ray,
                                          const Medium& currentMedium)
{
  const uniform Glass* uniform self = (const uniform Glass* uniform)material;

  float eta = eq(currentMedium, self->mediumOutside) ? self->mediumOutside.eta*rcp(self->mediumInside.eta)
                                                     : self->mediumInside.eta*rcp(self->mediumOutside.eta);

  return Dielectric_create(ctx, LinearSpace3f_create(ctx, frame(dg.Ns)), eta);
}

vec3f Glass_getTransparency(const uniform PathTraceMaterial* uniform material,
                            const DifferentialGeometry& dg,
                            const Ray& ray,
                            const Medium& currentMedium)
{
  const uniform Glass* uniform self = (const uniform Glass* uniform)material;

  float eta = eq(currentMedium, self->mediumOutside) ? self->mediumOutside.eta*rcp(self->mediumInside.eta)
                                                     : self->mediumInside.eta*rcp(self->mediumOutside.eta);

  float cosThetaO = max(dot(ray.dir, dg.Ns), 0.0f);
  return make_vec3f(1.0f-fresnelDielectric(cosThetaO, eta));
}

void Glass_selectNextMedium(const uniform PathTraceMaterial* uniform base,
                            varying Medium& currentMedium)
{
  const uniform Glass* uniform self = (const uniform Glass* uniform)base;

  if (eq(currentMedium, self->mediumOutside))
    currentMedium = self->mediumInside;
  else
    currentMedium = self->mediumOutside;
}

void Glass_Constructor(uniform Glass* uniform self,
                       uniform float etaInside,
                       const uniform vec3f& transmissionInside,
                       uniform float etaOutside,
                       const uniform vec3f& transmissionOutside)
{
  PathTraceMaterial_Constructor(&self->base, Glass_getBSDF, Glass_getTransparency, Glass_selectNextMedium);
  self->mediumInside.eta           = etaInside;
  self->mediumInside.transmission  = transmissionInside;
  self->mediumOutside.eta          = etaOutside;
  self->mediumOutside.transmission = transmissionOutside;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_Glass_create(uniform float etaInside,
                                             const uniform vec3f& transmissionInside,
                                             uniform float etaOutside,
                                             const uniform vec3f& transmissionOutside)
{
  uniform Glass* uniform self = uniform new uniform Glass;
  Glass_Constructor(self, etaInside, transmissionInside, etaOutside, transmissionOutside);
  return self;
}
