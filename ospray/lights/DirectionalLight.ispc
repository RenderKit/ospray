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

struct DirectionalLight {
  Light super;      //!< inherited light fields

  linear3f frame;   //!< coordinate frame, with vz == direction *towards* the light source
  vec3f radiance;   //!< RGB color and intensity of light
  float cosAngle;   //!< Angular limit of the cone light in an easier to use form: cosine of the half angle in radians
  float pdf;        //!< Probability to sample a direction to the light
};

// for very small cones treat as singular light, because float precision is not good enough 
#define COS_ANGLE_MAX 0.99999988f


// Implementation
//////////////////////////////////////////////////////////////////////////////

varying LightSample DirectionalLight_sample(const uniform Light *uniform _self,
                                            const varying DifferentialGeometry &dg,
                                            const varying vec2f &s)
{
  const DirectionalLight *uniform self = (DirectionalLight *uniform)_self;
  LightSample sample;

  sample.direction = self->frame.vz;
  sample.distance = inf;
  sample.pdf = self->pdf;

  if (self->cosAngle < COS_ANGLE_MAX)
    sample.direction = self->frame * uniformSampleCone(self->cosAngle, s);

  sample.radiance = self->radiance; // *pdf/pdf cancel

  return sample;
}

varying LightEval DirectionalLight_eval(const uniform Light *uniform _self,
    const varying vec3f &pos,
    const varying vec3f &dir)
{
  uniform DirectionalLight *uniform self = (uniform DirectionalLight *uniform)_self;
  LightEval res;
  res.distance = inf;

  if (self->cosAngle < COS_ANGLE_MAX && dot(self->frame.vz, dir) > self->cosAngle) {
    res.radiance = self->radiance * self->pdf;
    res.pdf = self->pdf;
  } else {
    res.radiance = make_vec3f(0.f);
    res.pdf = 0.f;
  }

  return res;
}


// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

//! Set the parameters of an ispc-side DirectionalLight object
export void DirectionalLight_set(void *uniform _self,
                                 const uniform vec3f &direction,
                                 const uniform vec3f &radiance,
                                 const uniform float cosAngle)
{
  uniform DirectionalLight *uniform self = (uniform DirectionalLight *uniform)_self;
  self->frame = frame(direction);
  self->radiance = radiance;
  self->cosAngle = cosAngle;
  self->pdf = cosAngle < COS_ANGLE_MAX ? uniformSampleConePDF(cosAngle) : inf;
}

//! Create an ispc-side DirectionalLight object
export void *uniform DirectionalLight_create(void *uniform cppEquivalent)
{
  uniform DirectionalLight *uniform self = uniform new uniform DirectionalLight;
  Light_Constructor(&self->super, cppEquivalent);
  self->super.sample = DirectionalLight_sample;
  self->super.eval = DirectionalLight_eval;

  DirectionalLight_set(self, make_vec3f(0.f, 0.f, 1.f), make_vec3f(1.f), 1.f);
  return self;
}
