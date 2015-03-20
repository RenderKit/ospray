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
#include "ospray/math/vec.ih"

struct SpotLight {
  uniform Light super;            //!< inherited light fields

  uniform vec3f position;         //!< Position of the SpotLight
  uniform vec3f direction;        //!< Direction that the SpotLight is emitting
  uniform vec3f power;            //!< RGB color and intensity of the SpotLight
  uniform float cosLimit;         //!< Angular limit of the spot in an easier to use form: cosine of the half angle in radians.
  uniform float range;            //!< Max influence range of the SpotLight
};


// Implementation
//////////////////////////////////////////////////////////////////////////////

varying vec3f SpotLight_computeRadiance(const uniform Light *uniform _self,
                                        const varying vec3f &coordinates,
                                        varying vec3f &direction,
                                        varying float &distance)
{
    const SpotLight *uniform self = (SpotLight *uniform)_self;

    // extant light vector from the hit point
    const vec3f L = self->position - coordinates;

    distance = length(L);

    // normalize light vector
    direction = L / distance;

    // cosine of the negated light direction and light vector.
    const float cosAngle = -dot(self->direction, direction);

    // TODO Angular attenuation factor.
    const float angularAttenuation = (cosAngle - self->cosLimit) / (1.0f - self->cosLimit);

    // convert from power to radiance by attenuting by distance^2; attentuate by angle and clip to the angular limit
    return(cosAngle > self->cosLimit ? self->power / (distance * distance) * angularAttenuation : make_vec3f(0.0f));
}

void SpotLight_Constructor(uniform SpotLight *uniform self,
                                  void *uniform cppEquivalent,
                                  const uniform vec3f &position,
                                  const uniform vec3f &direction,
                                  const uniform vec3f &power,
                                  const uniform float cosLimit,
                                  const uniform float range)
{
  Light_Constructor(&self->super, cppEquivalent);
  self->position     = position;
  self->direction    = direction;
  self->power        = power;
  self->cosLimit     = cosLimit;
  self->range        = range;
  self->super.computeRadiance = SpotLight_computeRadiance;
}

void SpotLight_Destructor(uniform SpotLight *uniform self)
{
  Light_Destructor(&self->super);
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
                        0.f,
                        inf);
  return self;
}

//! Destroy an ispc-side SpotLight object
export void SpotLight_destroy(void *uniform _self)
{
  uniform SpotLight *uniform self = (uniform SpotLight *uniform)_self;
  SpotLight_Destructor(self);
  delete self;
}

//! Set the parameters of an ispc-side SpotLight object
export void SpotLight_set(void *uniform _self,
                          const uniform vec3f &position,
                          const uniform vec3f &direction,
                          const uniform vec3f &power,
                          const uniform float cosLimit,
                          const uniform float range)
{
  uniform SpotLight *uniform self = (uniform SpotLight *uniform)_self;
  self->position     = position;
  self->direction    = direction;
  self->power        = power;
  self->cosLimit     = cosLimit;
  self->range        = range;
}
