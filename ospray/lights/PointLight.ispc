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
  const float dist2 = dot(dir, dir);
  const float invdist = rsqrt(dist2);

  // normalized light vector
  sample.direction = dir * invdist;
  sample.distance = dist2 * invdist;

  sample.pdf = 1.f; // per default we always take this sample

  // convert from power to radiance by attenuating by distance^2
  sample.radiance = self->power * (invdist * invdist);

  if (self->radius > 0.) {
    // sample surface of sphere as seen by hit point -> cone of directions
    const float sinTheta = self->radius * invdist;
    if (sinTheta < 1.f) {
      const float cosTheta = sqrt(1.f - sinTheta * sinTheta);
      const vec3f localDir = uniformSampleCone(cosTheta, s);
      sample.direction = frame(sample.direction) * localDir;
      sample.pdf = uniformSampleConePDF(cosTheta);
      const float c = localDir.z;
      sample.distance = c*sample.distance - sqrt(self->radius*self->radius - (1.f - c*c) * dist2);
    } else { // inside sphere
      const vec3f localDir = cosineSampleHemisphere(s);
      sample.direction = frame(dg.Ns) * localDir;
      sample.pdf = cosineSampleHemispherePDF(localDir);
      // TODO:
      sample.radiance = self->power * rcp(self->radius*self->radius);
      sample.distance = self->radius;
    }
  }

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
