// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.ih"
#include "common/Instance.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"

struct DirectionalLight
{
  Light super; //!< inherited light fields

  linear3f frame; //!< coordinate frame, with vz == direction *towards* the
                  //!< light source
  vec3f irradiance; //!< RGB irradiance contribution of the light
  float cosAngle; //!< Angular limit of the cone light in an easier to use form:
                  //!< cosine of the half angle in radians
  float pdf; //!< Probability to sample a direction to the light
};

// for very small cones treat as singular light, because float precision is not
// good enough
#define COS_ANGLE_MAX 0.99999988f

// Implementation
//////////////////////////////////////////////////////////////////////////////

inline Light_SampleRes Sample(const DirectionalLight *uniform self,
    const uniform linear3f &frame,
    const vec2f &s)
{
  Light_SampleRes res;

  res.dir = frame.vz;
  res.dist = inf;
  res.pdf = self->pdf;

  if (self->cosAngle < COS_ANGLE_MAX)
    res.dir = frame * uniformSampleCone(self->cosAngle, s);

  res.weight = self->irradiance; // *pdf/pdf cancel

  return res;
}

Light_SampleRes DirectionalLight_sample(const Light *uniform super,
    const DifferentialGeometry &,
    const vec2f &s,
    const float)
{
  const DirectionalLight *uniform self = (DirectionalLight * uniform) super;
  assert(self);
  return Sample(self, self->frame, s);
}

Light_SampleRes DirectionalLight_sample_instanced(const Light *uniform super,
    const DifferentialGeometry &,
    const vec2f &s,
    const float time)
{
  const DirectionalLight *uniform self = (DirectionalLight * uniform) super;
  assert(self);

  const Instance *uniform instance = self->super.instance;
  assert(instance);

  Light_SampleRes res;
  foreach_unique (utime in time) {
    const uniform affine3f xfm = Instance_getTransform(instance, utime);
    res = Sample(self, xfm.l * self->frame, s);
  }
  return res;
}

inline Light_EvalRes Eval(const DirectionalLight *uniform self,
    const uniform linear3f &frame,
    const vec3f &dir,
    const float maxDist)
{
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);

  if (inf <= maxDist && self->cosAngle < COS_ANGLE_MAX
      && dot(frame.vz, dir) > self->cosAngle) {
    res.radiance = self->irradiance * self->pdf;
    res.pdf = self->pdf;
  }

  return res;
}

Light_EvalRes DirectionalLight_eval(const Light *uniform super,
    const DifferentialGeometry &,
    const vec3f &dir,
    const float,
    const float maxDist,
    const float)
{
  const DirectionalLight *uniform self = (DirectionalLight * uniform) super;
  assert(self);
  return Eval(self, self->frame, dir, maxDist);
}

Light_EvalRes DirectionalLight_eval_instanced(const Light *uniform super,
    const DifferentialGeometry &,
    const vec3f &dir,
    const float,
    const float maxDist,
    const float time)
{
  const DirectionalLight *uniform self = (DirectionalLight * uniform) super;
  assert(self);

  const Instance *uniform instance = self->super.instance;
  assert(instance);

  Light_EvalRes res;
  foreach_unique (utime in time) {
    const uniform affine3f xfm = Instance_getTransform(instance, utime);
    res = Eval(self, xfm.l * self->frame, dir, maxDist);
  }
  return res;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

//! Set the parameters of an ispc-side DirectionalLight object
export void DirectionalLight_set(void *uniform super,
    const uniform vec3f &irradiance,
    const uniform vec3f &direction,
    uniform float cosAngle)
{
  DirectionalLight *uniform self = (DirectionalLight * uniform) super;
  self->frame = frame(direction);
  self->irradiance = irradiance;
  self->cosAngle = cosAngle;
  self->pdf = cosAngle < COS_ANGLE_MAX ? uniformSampleConePDF(cosAngle) : inf;

  // Enable dynamic runtime instancing or apply static transformation
  const Instance *uniform instance = self->super.instance;
  if (instance) {
    if (instance->motionBlur) {
      self->super.sample = DirectionalLight_sample_instanced;
      self->super.eval = DirectionalLight_eval_instanced;
    } else {
      self->frame = instance->xfm.l * self->frame;
    }
  }
}

//! Create an ispc-side DirectionalLight object
export void *uniform DirectionalLight_create()
{
  DirectionalLight *uniform self = uniform new uniform DirectionalLight;
  Light_Constructor(&self->super);
  self->super.sample = DirectionalLight_sample;
  self->super.eval = DirectionalLight_eval;

  DirectionalLight_set(
      self, make_vec3f(0.f, 0.f, 0.f), make_vec3f(0.f, 0.f, 1.f), 1.f);
  return self;
}
