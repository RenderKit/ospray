// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"

struct AmbientLight
{
  Light super; //!< inherited light fields
};

// Implementation
//////////////////////////////////////////////////////////////////////////////

// XXX importance sampling is only done into the positive hemisphere
// ==> poor support for translucent materials
Light_SampleRes AmbientLight_sample(const uniform Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &s)
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
  res.weight = self->super.radiance * rcp(res.pdf);

  return res;
}

Light_EvalRes AmbientLight_eval(const uniform Light *uniform super,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float,
    const float maxDist)
{
  uniform AmbientLight *uniform self = (uniform AmbientLight * uniform) super;
  Light_EvalRes res;

  res.radiance = inf <= maxDist ? self->super.radiance : make_vec3f(0.f);
  if (reduce_max(abs(dg.Ns)) > 0)
    res.pdf = cosineSampleHemispherePDF(max(dot(dg.Ns, dir), 0.f));
  else
    res.pdf = uniformSampleSpherePDF(1.f);

  return res;
}

void AmbientLight_Constructor(uniform AmbientLight *uniform self)
{
  Light_Constructor(&self->super);
  self->super.sample = AmbientLight_sample;
  self->super.eval = AmbientLight_eval;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

//! Create an ispc-side AmbientLight object
export void *uniform AmbientLight_create()
{
  uniform AmbientLight *uniform self = uniform new uniform AmbientLight;
  AmbientLight_Constructor(self);
  return self;
}

//! Set the parameters of an ispc-side AmbientLight object
export void AmbientLight_set(void *uniform super)
{
  uniform AmbientLight *uniform self = (uniform AmbientLight * uniform) super;
}
