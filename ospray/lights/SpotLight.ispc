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

struct SpotLight {
  Light super;            //!< inherited light fields

  vec3f position;         //!< Position of the SpotLight
  vec3f direction;        //!< Direction that the SpotLight is emitting; normalized
  vec3f power;            //!< RGB color and intensity of the SpotLight
  float cosAngleMax;      //!< Angular limit of the spot in an easier to use form: cosine of the half angle in radians
  float cosAngleMin;      //!< Border of the penumbra area of the spot in an easier to use form; larger than cosAngleMax
  float radius;           //!< defines the size of the (extended) SpotLight
};


// Implementation
//////////////////////////////////////////////////////////////////////////////

varying LightSample SpotLight_sample(const uniform Light *uniform _self,
                                     const varying DifferentialGeometry &dg,
                                     const varying vec2f &s)
{
  const SpotLight *uniform self = (SpotLight *uniform)_self;
  LightSample sample;

  // extant light vector from the hit point
  sample.direction = self->position - dg.P;

  if (self->radius > 0.)
    sample.direction = frame(self->direction) * uniformSampleDisk(self->radius, s) + sample.direction;

  const float dist2 = dot(sample.direction, sample.direction);
  const float invdist = rsqrt(dist2);

  // normalized light vector
  sample.direction = sample.direction * invdist;
  sample.distance = dist2 * invdist;

  // cosine of the negated light direction and light vector.
  const float cosAngle = -dot(self->direction, sample.direction);
  const float angularAttenuation = clamp((cosAngle - self->cosAngleMax) * rcp(self->cosAngleMin - self->cosAngleMax));

  if (self->radius > 0.)
    sample.pdf = uniformSampleDiskPDF(self->radius) * dist2 * abs(cosAngle);
  else
    sample.pdf = inf; // we always take this sample

  // convert from power to radiance by attenuating by distance^2; attenuate by angle
  sample.radiance = self->power * (invdist * invdist * angularAttenuation);

  return sample;
}


// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

//! Set the parameters of an ispc-side SpotLight object
export void SpotLight_set(void *uniform _self,
                          const uniform vec3f &position,
                          const uniform vec3f &direction,
                          const uniform vec3f &power,
                          const uniform float cosAngleMax,
                          const uniform float cosAngleMin,
                          const uniform float radius)
{
  uniform SpotLight *uniform self = (uniform SpotLight *uniform)_self;
  self->position     = position;
  self->direction    = direction;
  self->power        = power;
  self->cosAngleMax  = cosAngleMax;
  self->cosAngleMin  = cosAngleMin;
  self->radius       = radius;
}

//! Create an ispc-side SpotLight object
export void *uniform SpotLight_create(void *uniform cppEquivalent)
{
  uniform SpotLight *uniform self = uniform new uniform SpotLight;

  Light_Constructor(&self->super, cppEquivalent);
  self->super.sample = SpotLight_sample;

  SpotLight_set(self,
                make_vec3f(0.f),
                make_vec3f(0.f, 0.f, 1.f),
                make_vec3f(1.f),
                0.f,
                0.01f,
                0.f);

  return self;
}
