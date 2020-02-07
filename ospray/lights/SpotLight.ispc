// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"

struct SpotLight
{
  Light super; //!< inherited light fields

  vec3f position; //!< Position of the SpotLight
  linear3f frame; //!< coordinate frame, with vz == direction that the SpotLight
                  //!< is emitting
  vec3f power; //!< RGB color and intensity of the SpotLight
  float cosAngleMax; //!< Angular limit of the spot in an easier to use form:
                     //!< cosine of the half angle in radians
  float cosAngleScale; //!< 1/(cos(border of the penumbra area) - cosAngleMax);
                       //!< positive
  float radius; //!< defines the size of the (extended) SpotLight
  float diskPdf; //!< pdf of disk with radius
};

// Implementation
//////////////////////////////////////////////////////////////////////////////

Light_SampleRes SpotLight_sample(const uniform Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &s)
{
  const SpotLight *uniform self = (SpotLight * uniform) super;
  Light_SampleRes res;

  // extant light vector from the hit point
  res.dir = self->position - dg.P;

  if (self->radius > 0.)
    res.dir = self->frame * uniformSampleDisk(self->radius, s) + res.dir;

  const float dist2 = dot(res.dir, res.dir);
  const float invdist = rsqrt(dist2);

  // normalized light vector
  res.dir = res.dir * invdist;
  res.dist = dist2 * invdist;

  // cosine of the negated light direction and light vector.
  const float cosAngle = -dot(self->frame.vz, res.dir);
  const float angularAttenuation =
      clamp((cosAngle - self->cosAngleMax) * self->cosAngleScale);

  if (self->radius > 0.)
    res.pdf = self->diskPdf * dist2 * abs(cosAngle);
  else
    res.pdf = inf; // we always take this sample

  // convert from power to radiance by attenuating by distance^2; attenuate by
  // angle
  res.weight = self->power * (sqr(invdist) * angularAttenuation);

  return res;
}

Light_EvalRes SpotLight_eval(const uniform Light *uniform super,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist)
{
  const SpotLight *uniform self = (SpotLight * uniform) super;
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);

  if (self->radius > 0.f) {
    // intersect disk
    const float cosAngle = -dot(dir, self->frame.vz);
    if (cosAngle > self->cosAngleMax) { // inside illuminated cone?
      const vec3f vp = dg.P - self->position;
      const float t = dot(vp, self->frame.vz) * rcp(cosAngle);
      if (t > minDist & t <= maxDist) {
        const vec3f vd = vp + t * dir;
        if (dot(vd, vd) < sqr(self->radius)) { // inside disk?
          const float angularAttenuation =
              min((cosAngle - self->cosAngleMax) * self->cosAngleScale, 1.f);
          const float pdf = self->diskPdf * cosAngle;
          res.radiance = self->power
              * (angularAttenuation * pdf); // *sqr(t)/sqr(t) cancels
          res.pdf = pdf * sqr(t);
        }
      }
    }
  }

  return res;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

//! Set the parameters of an ispc-side SpotLight object
export void SpotLight_set(void *uniform super,
    const uniform vec3f &position,
    const uniform vec3f &direction,
    uniform float cosAngleMax,
    uniform float cosAngleScale,
    uniform float radius)
{
  uniform SpotLight *uniform self = (uniform SpotLight * uniform) super;
  self->position = position;
  self->frame = frame(direction);
  self->power = self->super.radiance;
  self->cosAngleMax = cosAngleMax;
  self->cosAngleScale = cosAngleScale;
  self->radius = radius;
  self->diskPdf = uniformSampleDiskPDF(radius);
}

//! Create an ispc-side SpotLight object
export void *uniform SpotLight_create()
{
  uniform SpotLight *uniform self = uniform new uniform SpotLight;

  Light_Constructor(&self->super);
  self->super.sample = SpotLight_sample;
  self->super.eval = SpotLight_eval;

  SpotLight_set(
      self, make_vec3f(0.f), make_vec3f(0.f, 0.f, 1.f), 0.f, 100.f, 0.f);

  return self;
}
