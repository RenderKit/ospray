// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/Instance.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"
// c++ shared
#include "DirectionalLightShared.h"

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

export void *uniform DirectionalLight_sample_addr()
{
  return DirectionalLight_sample;
}

export void *uniform DirectionalLight_sample_instanced_addr()
{
  return DirectionalLight_sample_instanced;
}

export void *uniform DirectionalLight_eval_addr()
{
  return DirectionalLight_eval;
}

export void *uniform DirectionalLight_eval_instanced_addr()
{
  return DirectionalLight_eval_instanced;
}