// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"

struct DirectionalLight
{
  Light super; //!< inherited light fields

  linear3f frame; //!< coordinate frame, with vz == direction *towards* the
                  //!< light source
  vec3f power; //!< RGB power of light
  float cosAngle; //!< Angular limit of the cone light in an easier to use form:
                  //!< cosine of the half angle in radians
  float pdf; //!< Probability to sample a direction to the light
};

// for very small cones treat as singular light, because float precision is not
// good enough
#define COS_ANGLE_MAX 0.99999988f

// Implementation
//////////////////////////////////////////////////////////////////////////////

Light_SampleRes DirectionalLight_sample(const uniform Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &s)
{
  const DirectionalLight *uniform self = (DirectionalLight * uniform) super;
  Light_SampleRes res;

  res.dir = self->frame.vz;
  res.dist = inf;
  res.pdf = self->pdf;

  if (self->cosAngle < COS_ANGLE_MAX)
    res.dir = self->frame * uniformSampleCone(self->cosAngle, s);

  res.weight = self->power; // *pdf/pdf cancel

  return res;
}

Light_EvalRes DirectionalLight_eval(const uniform Light *uniform super,
    const DifferentialGeometry &,
    const vec3f &dir,
    const float,
    const float maxDist)
{
  uniform DirectionalLight *uniform self =
      (uniform DirectionalLight * uniform) super;
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);

  if (inf <= maxDist && self->cosAngle < COS_ANGLE_MAX
      && dot(self->frame.vz, dir) > self->cosAngle) {
    res.radiance = self->power * self->pdf;
    res.pdf = self->pdf;
  }

  return res;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

//! Set the parameters of an ispc-side DirectionalLight object
export void DirectionalLight_set(
    void *uniform super, const uniform vec3f &direction, uniform float cosAngle)
{
  uniform DirectionalLight *uniform self =
      (uniform DirectionalLight * uniform) super;
  self->frame = frame(direction);
  self->power = self->super.radiance;
  self->cosAngle = cosAngle;
  self->pdf = cosAngle < COS_ANGLE_MAX ? uniformSampleConePDF(cosAngle) : inf;
}

//! Create an ispc-side DirectionalLight object
export void *uniform DirectionalLight_create()
{
  uniform DirectionalLight *uniform self = uniform new uniform DirectionalLight;
  Light_Constructor(&self->super);
  self->super.sample = DirectionalLight_sample;
  self->super.eval = DirectionalLight_eval;

  DirectionalLight_set(self, make_vec3f(0.f, 0.f, 1.f), 1.f);
  return self;
}
