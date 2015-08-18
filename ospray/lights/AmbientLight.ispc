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

#include "Light.ih"
#include "ospray/math/sampling.ih"
#include "ospray/math/LinearSpace.ih"

struct AmbientLight {
  uniform Light super;      //!< inherited light fields

  uniform vec3f radiance;   //!< RGB color and intensity of light
};


// Implementation
//////////////////////////////////////////////////////////////////////////////

varying LightSample AmbientLight_sample(const uniform Light *uniform _self,
                                        const varying DifferentialGeometry &dg,
                                        const varying vec2f &s)
{
  uniform AmbientLight *uniform self = (uniform AmbientLight *uniform)_self;
  LightSample sample;

  const vec3f localDir = cosineSampleHemisphere(s);
  sample.direction = frame(dg.Ns) * localDir;
  sample.pdf = cosineSampleHemispherePDF(localDir);
  sample.distance = inf;
  sample.radiance = self->radiance;

  return sample;
}

varying vec3f AmbientLight_evalEnv(const uniform Light *uniform _self,
                                   const varying vec3f &)
{
  uniform AmbientLight *uniform self = (uniform AmbientLight *uniform)_self;
  return self->radiance;
}


void AmbientLight_Constructor(uniform AmbientLight *uniform self,
                              void *uniform cppEquivalent,
                              const uniform vec3f &radiance)
{
  Light_Constructor(&self->super, cppEquivalent);
  self->radiance = radiance;
  self->super.sample = AmbientLight_sample;
  self->super.evalEnv = AmbientLight_evalEnv;
}


// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

//! Create an ispc-side AmbientLight object
export void *uniform AmbientLight_create(void *uniform cppEquivalent)
{
  uniform AmbientLight *uniform self = uniform new uniform AmbientLight;
  AmbientLight_Constructor(self, cppEquivalent, make_vec3f(1.f));
  return self;
}

//! Set the parameters of an ispc-side AmbientLight object
export void AmbientLight_set(void *uniform _self,
                             const uniform vec3f &radiance)
{
  uniform AmbientLight *uniform self = (uniform AmbientLight *uniform)_self;
  self->radiance = radiance;
}
