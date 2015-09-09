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

struct PointLight {
  Light super;    //!< inherited light fields

  vec3f position; //!< light position
  vec3f power;    //!< RGB color and intensity of light
  float radius;   //!< defines the size of the SphereLight
};


// Implementation
//////////////////////////////////////////////////////////////////////////////

varying LightSample PointLight_sample(const uniform Light *uniform _self,
                                      const varying DifferentialGeometry &dg,
                                      const varying vec2f &s)
{
  const PointLight *uniform self = (PointLight *uniform)_self;
  LightSample sample;

  // extant light vector from the hit point
  const vec3f dir = self->position - dg.P;
  const float dist = length(dir);

  // normalized light vector
  sample.direction = dir / dist;
  sample.distance = dist;

  if (self->radius > 0.) {
    // sample surface of sphere as seen by hit point -> cone of directions
    const float sinTheta = self->radius / dist;
    const float cosTheta = sin2cos(sinTheta);
    sample.direction = frame(sample.direction) * uniformSampleCone(cosTheta, s);
    sample.pdf = uniformSampleConePDF(cosTheta);
    // TODO: shorten dist
  } else {
    // we always take this sample
    sample.pdf = 1.f;
  }

  // convert from power to radiance by attenuating by distance^2
  sample.radiance = self->power * rcp(dist * dist);

  return sample;
}


// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

//! Set the parameters of an ispc-side PointLight object
export void PointLight_set(void *uniform _self,
                           const uniform vec3f &position,
                           const uniform vec3f &power,
                           const uniform float radius)
{
  uniform PointLight *uniform self = (uniform PointLight *uniform)_self;
  self->position = position;
  self->power = power;
  self->radius = radius;
}

//! Create an ispc-side PointLight object
export void *uniform PointLight_create(void *uniform cppEquivalent)
{
  uniform PointLight *uniform self = uniform new uniform PointLight;
  Light_Constructor(&self->super, cppEquivalent);
  self->super.sample = PointLight_sample;

  PointLight_set(self, make_vec3f(0.f), make_vec3f(1.f), 0.f);
  return self;
}
