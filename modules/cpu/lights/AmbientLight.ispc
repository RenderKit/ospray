// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/DifferentialGeometry.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"
// c++ shared
#include "AmbientLightShared.h"

// Implementation
//////////////////////////////////////////////////////////////////////////////

// XXX importance sampling is only done into the positive hemisphere
// ==> poor support for translucent materials
Light_SampleRes AmbientLight_sample(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &s,
    const float)
{
  uniform AmbientLight *uniform self = (uniform AmbientLight * uniform) super;
  Light_SampleRes res;

  if (reduce_max(abs(dg.Ns)) > 0) {
    const vec3f localDir = cosineSampleHemisphere(s);
    res.dir = frame(dg.Ns) * localDir;
    res.pdf = cosineSampleHemispherePDF(localDir);
  } else {
    res.dir = uniformSampleSphere(1.f, s);
    res.pdf = uniformSampleSpherePDF(1.f);
  }
  res.dist = inf;
  res.weight = self->radiance * rcp(res.pdf);

  return res;
}

Light_EvalRes AmbientLight_eval(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float,
    const float maxDist,
    const float)
{
  uniform AmbientLight *uniform self = (uniform AmbientLight * uniform) super;
  Light_EvalRes res;

  res.radiance = inf <= maxDist ? self->radiance : make_vec3f(0.f);
  if (reduce_max(abs(dg.Ns)) > 0)
    res.pdf = cosineSampleHemispherePDF(max(dot(dg.Ns, dir), 0.f));
  else
    res.pdf = uniformSampleSpherePDF(1.f);

  return res;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void *uniform AmbientLight_sample_addr()
{
  return AmbientLight_sample;
}

export void *uniform AmbientLight_eval_addr()
{
  return AmbientLight_eval;
}