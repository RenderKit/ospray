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

struct PointLight {
  uniform Light super;    //!< inherited light fields

  uniform vec3f position; //!< light position
  uniform vec3f power;    //!< RGB color and intensity of light
  uniform float range;    //!< maximum range at which the light will have an effect
};


// Implementation
//////////////////////////////////////////////////////////////////////////////

varying vec3f PointLight_computeRadiance(const uniform Light *uniform _self,
                                         const varying vec3f &coordinates,
                                         varying vec3f &direction,
                                         varying float &distance)
{
    const PointLight *uniform self = (PointLight *uniform)_self;

    // extant light vector from the hit point
    const vec3f L = self->position - coordinates;

    distance = length(L);

    // normalize light vector
    direction = L / distance;

    // TODO: account for range?

    // convert from power to radiance by attenuting by distance^2
    return self->power / (distance * distance);
}

void PointLight_Constructor(uniform PointLight *uniform self,
                            void *uniform cppEquivalent,
                            const uniform vec3f &position,
                            const uniform vec3f &power,
                            const uniform float range)
{
  Light_Constructor(&self->super, cppEquivalent);
  self->position = position;
  self->power    = power;
  self->range    = range;
  self->super.computeRadiance = PointLight_computeRadiance;
}

void PointLight_Destructor(uniform PointLight *uniform self)
{
  Light_Destructor(&self->super);
}


// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

//! Create an ispc-side PointLight object
export void *uniform PointLight_create(void *uniform cppEquivalent)
{
  uniform PointLight *uniform self = uniform new uniform PointLight;
  PointLight_Constructor(self, cppEquivalent, make_vec3f(0.f), make_vec3f(1.f), inf);
  return self;
}

//! Destroy an ispc-side PointLight object
export void PointLight_destroy(void *uniform _self)
{
  uniform PointLight *uniform self = (uniform PointLight *uniform)_self;
  PointLight_Destructor(self);
  delete self;
}

//! Set the parameters of an ispc-side PointLight object
export void PointLight_set(void *uniform _self,
                           const uniform vec3f &position,
                           const uniform vec3f &power,
                           const uniform float range)
{
  uniform PointLight *uniform self = (uniform PointLight *uniform)_self;
  self->position = position;
  self->power = power;
  self->range = range;
}
