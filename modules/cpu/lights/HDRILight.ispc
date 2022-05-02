// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/Instance.ih"
#include "math/Distribution2D.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"
#include "texture/Texture2D.ih"
// c++ shared
#include "HDRILightShared.h"

// Implementation
//////////////////////////////////////////////////////////////////////////////

// sample function used when no environment map is given: black
Light_SampleRes HDRILight_sample_dummy(const Light *uniform,
    const DifferentialGeometry &,
    const vec2f &,
    const float)
{
  Light_SampleRes res;
  memset(&res, 0, sizeof(Light_SampleRes));
  return res;
}

inline Light_SampleRes Sample(const HDRILight *uniform self,
    const uniform linear3f &light2world,
    const vec2f &s)
{
  Light_SampleRes res;

  Sample2D sample2d = Distribution2D_sample(self->distribution, s);
  // Distribution2D samples within bin i as (i, i+1), whereas we provided
  // average importance for (i-0.5, i+0.5), thus shift by 0.5
  sample2d.uv = sample2d.uv - self->map->halfTexel;

  const float phi = two_pi * sample2d.uv.x;
  const float theta = M_PI * sample2d.uv.y;

  float sinTheta, cosTheta;
  sincos(theta, &sinTheta, &cosTheta);
  const vec3f localDir = cartesian(phi, sinTheta, cosTheta);

  res.dir = light2world * localDir;

  res.pdf = sample2d.pdf * one_over_two_pi_sqr * rcp(sinTheta);

  res.dist = inf;

  // clamp2edge for theta for tex lookup, to prevent light leaks at the poles
  sample2d.uv.y = clamp(
      sample2d.uv.y, self->map->halfTexel.y, 1.0f - self->map->halfTexel.y);
  DifferentialGeometry lookup;
  initDgFromTexCoord(lookup, sample2d.uv);
  res.weight = get3f(self->map, lookup) * self->radianceScale / res.pdf;

  return res;
}

Light_SampleRes HDRILight_sample(const Light *uniform super,
    const DifferentialGeometry &,
    const vec2f &s,
    const float)
{
  const HDRILight *uniform self = (HDRILight * uniform) super;
  assert(self);
  return Sample(self, self->light2world, s);
}

Light_SampleRes HDRILight_sample_instanced(const Light *uniform super,
    const DifferentialGeometry &,
    const vec2f &s,
    const float time)
{
  const HDRILight *uniform self = (HDRILight * uniform) super;
  assert(self);

  const Instance *uniform instance = self->super.instance;
  assert(instance);

  Light_SampleRes res;
  foreach_unique (utime in time) {
    const uniform affine3f xfm = Instance_getTransform(instance, utime);
    res = Sample(self, xfm.l * self->light2world, s);
  }
  return res;
}

inline Light_EvalRes Eval(HDRILight *uniform self,
    const uniform linear3f &world2light,
    const vec3f &dir,
    const float maxDist)
{
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);

  if (inf > maxDist)
    return res;

  const vec3f localDir = world2light * dir;

  const float u = atan2(localDir.y, localDir.x) * one_over_two_pi;
  const float v = acos(localDir.z) * one_over_pi;
  const vec2f uv = make_vec2f(u, v);

  // clamp2edge for theta for tex lookup, to prevent light leaks at the poles
  const vec2f uvc = make_vec2f(
      u, clamp(v, self->map->halfTexel.y, 1.0f - self->map->halfTexel.y));
  DifferentialGeometry lookup;
  initDgFromTexCoord(lookup, uvc);
  res.radiance = get3f(self->map, lookup) * self->radianceScale;

  // domain of Distribution2D is shifted by half a texel compared to texture
  // atan2 can get negative, shift can lead to values > 1.f: reproject to [0..1)
  const vec2f uvd = frac(uv + self->map->halfTexel);
  res.pdf = Distribution2D_pdf(self->distribution, uvd);
  res.pdf *= one_over_two_pi_sqr * rsqrt(1.f - sqr(localDir.z));

  return res;
}

Light_EvalRes HDRILight_eval(const Light *uniform super,
    const DifferentialGeometry &,
    const vec3f &dir,
    const float,
    const float maxDist,
    const float)
{
  const HDRILight *uniform self = (HDRILight * uniform) super;
  assert(self);
  return Eval(self, self->world2light, dir, maxDist);
}

Light_EvalRes HDRILight_eval_instanced(const Light *uniform super,
    const DifferentialGeometry &,
    const vec3f &dir,
    const float,
    const float maxDist,
    const float time)
{
  const HDRILight *uniform self = (HDRILight * uniform) super;
  assert(self);

  const Instance *uniform instance = self->super.instance;
  assert(instance);

  Light_EvalRes res;
  foreach_unique (utime in time) {
    const uniform affine3f xfm = Instance_getTransform(instance, utime);
    res = Eval(self, self->world2light * rcp(xfm.l), dir, maxDist);
  }
  return res;
}

// bin i represents the average contribution of (i-0.5, i+0.5) when we sample
// the texture bilinearly at i
// for i==0 we have a wrap-around, which is wanted for x (phi), but actually
// not for y (theta), because then light (importance) from the south-pole is
// leaking to the north-pole
// however, sin(theta) is zero then, thus we will never sample there
task unmasked void HDRILight_calcRowImportance(
    const Texture2D *uniform const map,
    float *uniform const importance,
    float *uniform const row_importance)
{
  const uniform int y = taskIndex;
  const uniform vec2f rcpSize = 1.f / map->sizef;
  const uniform float fy = y * rcpSize.y;
  const uniform int width = map->size.x;
  const uniform float sinTheta = sin(fy * M_PI);
  foreach (x = 0 ... width) {
    const vec2f coord = make_vec2f(x * rcpSize.x, fy);
    // using bilinear filtering is indeed what we want
    DifferentialGeometry lookup;
    initDgFromTexCoord(lookup, coord);
    const vec3f col = get3f(map, lookup);
    importance[y * width + x] = sinTheta * luminance(col);
  }
  row_importance[y] = Distribution1D_create(width, importance + y * width);
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void *uniform HDRILight_sample_dummy_addr()
{
  return HDRILight_sample_dummy;
}

export void *uniform HDRILight_sample_addr()
{
  return HDRILight_sample;
}

export void *uniform HDRILight_sample_instanced_addr()
{
  return HDRILight_sample_instanced;
}

export void *uniform HDRILight_eval_addr()
{
  return HDRILight_eval;
}

export void *uniform HDRILight_eval_instanced_addr()
{
  return HDRILight_eval_instanced;
}

export void *uniform HDRILight_createDistribution(const void *uniform map)
{
  // calculate importance in parallel
  const Texture2D *uniform m = (const Texture2D *uniform)map;
  const uniform int height = m->size.y;
  float *uniform cdf_x = uniform new float[m->size.x * height];
  float *uniform row_importance = uniform new float[height];
  launch[height] HDRILight_calcRowImportance(m, cdf_x, row_importance);
  sync;

  // create distribution
  return Distribution2D_create(m->size, cdf_x, row_importance);
  // no delete[] (row_)importance: ownership was transferred to Distribution2D
}

export void HDRILight_destroyDistribution(void *uniform distribution)
{
  Distribution2D_destroy((Distribution2D * uniform) distribution);
}
