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
#include "../bsdfs/Lambert.ih"

struct Matte
{
  PathTraceMaterial super;
  vec3f reflectance;
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

const varying BSDF* uniform Matte_getBSDF(const uniform PathTraceMaterial* uniform super,
                                          uniform ShadingContext* uniform ctx,
                                          const DifferentialGeometry& dg,
                                          const Ray& ray,
                                          const Medium& currentMedium)
{
  const uniform Matte* uniform self = (const uniform Matte* uniform)super;
  vec3f R = self->reflectance;
  return Lambert_create(ctx, LinearSpace3f_create(ctx, frame(dg.Ns)), R);
}

inline void Matte_Constructor(uniform Matte* uniform self,
                              const uniform vec3f& reflectance)
{
  PathTraceMaterial_Constructor(&self->super, Matte_getBSDF);
  self->reflectance = reflectance;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform PathTracer_Matte_create(const uniform vec3f& reflectance)
{
  uniform Matte* uniform self = uniform new uniform Matte;
  Matte_Constructor(self, reflectance);
  return self;
}
