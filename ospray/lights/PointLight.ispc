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

  sample.pdf = inf; // per default we always take this sample

  // convert from power to radiance by attenuating by distance^2
  sample.weight = self->power * sqr(invdist);
  const float sinTheta = self->radius * invdist;

  if (self->radius > 0.f & sinTheta > 0.005f) {
    // sample surface of sphere as seen by hit point -> cone of directions
    // for very small cones treat as point light, because float precision is not good enough
    if (sinTheta < 1.f) {
      const float cosTheta = sqrt(1.f - sinTheta * sinTheta);
      const vec3f localDir = uniformSampleCone(cosTheta, s);
      sample.direction = frame(sample.direction) * localDir;
      sample.pdf = uniformSampleConePDF(cosTheta);
      const float c = localDir.z;
      sample.distance = c*sample.distance - sqrt(sqr(self->radius) - (1.f - c*c) * dist2);
      // TODO scale radiance by actual distance
    } else { // inside sphere
      const vec3f localDir = cosineSampleHemisphere(s);
      sample.direction = frame(dg.Ns) * localDir;
      sample.pdf = cosineSampleHemispherePDF(localDir);
      // TODO:
      sample.weight = self->power * rcp(sqr(self->radius));
      sample.distance = self->radius;
    }
  }

  return sample;
}

varying LightEval PointLight_eval(const uniform Light *uniform _self,
    const varying DifferentialGeometry &dg,
    const varying vec3f &dir)
{
  const PointLight *uniform self = (PointLight *uniform)_self;
  LightEval res;
  res.value = make_vec3f(0.f);
  res.distance = inf;
  res.pdf = 0.f;

  if (self->radius > 0.f) {
    const vec3f A = self->position - dg.P;
    const float a = dot(dir, dir);
    const float b = 2.f * dot(dir, A);
    const float centerDist2 = dot(A, A);
    const float c = centerDist2 - sqr(self->radius);
    const float radical = sqr(b) - 4.f*a*c;

    if (radical > 0.f) {
      const float t_near = (b - sqrt(radical)) / (2.f*a);
      const float t_far = (b + sqrt(radical)) / (2.f*a);

      if (t_far > 0.0f) {
        // TODO: handle interior case
        res.distance = t_near;
        const float sinTheta2 = sqr(self->radius) * rcp(centerDist2);
        const float cosTheta = sqrt(1.f - sinTheta2);
        res.pdf = uniformSampleConePDF(cosTheta);
        const float invdist = rcp(t_near);
        res.value = self->power * res.pdf * sqr(invdist);
      }
    }
  }

  return res;
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
export void *uniform PointLight_create()
{
  uniform PointLight *uniform self = uniform new uniform PointLight;
  Light_Constructor(&self->super);
  self->super.sample = PointLight_sample;
  self->super.eval = PointLight_eval;

  PointLight_set(self, make_vec3f(0.f), make_vec3f(1.f), 0.f);
  return self;
}
