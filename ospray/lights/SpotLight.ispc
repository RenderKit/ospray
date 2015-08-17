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

struct SpotLight {
  uniform Light super;            //!< inherited light fields

  uniform vec3f position;         //!< Position of the SpotLight
  uniform vec3f direction;        //!< Direction that the SpotLight is emitting
  uniform vec3f power;            //!< RGB color and intensity of the SpotLight
  uniform float cosLimit;         //!< Angular limit of the spot in an easier to use form: cosine of the half angle in radians.
};


// Implementation
//////////////////////////////////////////////////////////////////////////////

varying LightSample SpotLight_sample(const uniform Light *uniform _self,
                                     const varying DifferentialGeometry &dg,
                                     const varying vec2f &)
{
  const SpotLight *uniform self = (SpotLight *uniform)_self;
  LightSample sample;

  // extant light vector from the hit point
  const vec3f dir = self->position - dg.P;
  const float dist = length(dir);

  // normalized light vector
  sample.direction = dir / dist;
  sample.distance = dist;

  // we always take this sample
  sample.pdf = 1.f;

  // cosine of the negated light direction and light vector.
  const float cosAngle = -dot(self->direction, sample.direction);

  // TODO Angular attenuation factor.
  const float angularAttenuation = (cosAngle - self->cosLimit) / (1.f - self->cosLimit);

  // convert from power to radiance by attenuting by distance^2; attentuate by angle and clip to the angular limit
  sample.radiance = cosAngle > self->cosLimit ? self->power / (dist * dist) * angularAttenuation : make_vec3f(0.f);

  return sample;
}

void SpotLight_Constructor(uniform SpotLight *uniform self,
                                  void *uniform cppEquivalent,
                                  const uniform vec3f &position,
                                  const uniform vec3f &direction,
                                  const uniform vec3f &power,
                                  const uniform float cosLimit)
{
  Light_Constructor(&self->super, cppEquivalent);
  self->position     = position;
  self->direction    = direction;
  self->power        = power;
  self->cosLimit     = cosLimit;
  self->super.sample = SpotLight_sample;
}


// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

//! Create an ispc-side SpotLight object
export void *uniform SpotLight_create(void *uniform cppEquivalent)
{
  uniform SpotLight *uniform self = uniform new uniform SpotLight;
  SpotLight_Constructor(self,
                        cppEquivalent,
                        make_vec3f(0.f),
                        make_vec3f(0.f, 0.f, 1.f),
                        make_vec3f(1.f),
                        0.f);
  return self;
}

//! Set the parameters of an ispc-side SpotLight object
export void SpotLight_set(void *uniform _self,
                          const uniform vec3f &position,
                          const uniform vec3f &direction,
                          const uniform vec3f &power,
                          const uniform float cosLimit)
{
  uniform SpotLight *uniform self = (uniform SpotLight *uniform)_self;
  self->position     = position;
  self->direction    = direction;
  self->power        = power;
  self->cosLimit     = cosLimit;
}
