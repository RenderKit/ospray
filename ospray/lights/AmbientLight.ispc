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

#include "Light.ih"
#include "ospray/math/sampling.ih"
#include "ospray/math/LinearSpace.ih"

struct AmbientLight {
  Light super;      //!< inherited light fields

  vec3f radiance;   //!< RGB color and intensity of light
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
  sample.weight = self->radiance * rcp(sample.pdf);

  return sample;
}

varying LightEval AmbientLight_eval(const uniform Light *uniform _self,
    const varying DifferentialGeometry &,
    const varying vec3f &)
{
  uniform AmbientLight *uniform self = (uniform AmbientLight *uniform)_self;
  LightEval res;

  res.value = self->radiance;
  res.distance = inf;
  res.pdf = 1.f/(4.f * M_PI); // uniformSampleSpherePDF(); // XXX inconsistent to sample, need surface normal here

  return res;
}


void AmbientLight_Constructor(uniform AmbientLight *uniform self,
                              const uniform vec3f &radiance)
{
  Light_Constructor(&self->super);
  self->radiance = radiance;
  self->super.sample = AmbientLight_sample;
  self->super.eval = AmbientLight_eval;
}


// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

//! Create an ispc-side AmbientLight object
export void *uniform AmbientLight_create()
{
  uniform AmbientLight *uniform self = uniform new uniform AmbientLight;
  AmbientLight_Constructor(self, make_vec3f(1.f));
  return self;
}

//! Set the parameters of an ispc-side AmbientLight object
export void AmbientLight_set(void *uniform _self,
                             const uniform vec3f &radiance)
{
  uniform AmbientLight *uniform self = (uniform AmbientLight *uniform)_self;
  self->radiance = radiance;
}
