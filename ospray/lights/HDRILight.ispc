// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.ih"
#include "math/Distribution2D.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"
#include "texture/Texture2D.ih"

struct HDRILight
{
  Light super;

  linear3f light2world;
  linear3f world2light;
  Texture2D *uniform map; // Environment map in latitude / longitude format
  Distribution2D *uniform
      distribution; // The 2D distribution used to importance sample
  vec2f rcpSize; // precomputed 1/map.size
};

// Implementation
//////////////////////////////////////////////////////////////////////////////

// sample function used when no environment map is given: black
Light_SampleRes HDRILight_sample_dummy(
    const uniform Light *uniform, const DifferentialGeometry &, const vec2f &)
{
  Light_SampleRes res;
  memset(&res, 0, sizeof(Light_SampleRes));
  return res;
}

Light_SampleRes HDRILight_sample(const uniform Light *uniform super,
    const DifferentialGeometry &,
    const vec2f &s)
{
  uniform HDRILight *uniform self = (uniform HDRILight * uniform) super;
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

  res.dir = self->light2world * localDir;

  res.pdf = sample2d.pdf * one_over_two_pi_sqr * rcp(sinTheta);

  res.dist = inf;

  DifferentialGeometry lookup;
  initDgFromTexCoord(lookup, sample2d.uv);
  res.weight = get3f(self->map, lookup) * self->super.radiance / res.pdf;

  return res;
}

Light_EvalRes HDRILight_eval(const uniform Light *uniform super,
    const DifferentialGeometry &,
    const vec3f &dir,
    const float,
    const float maxDist)
{
  uniform HDRILight *uniform self = (uniform HDRILight * uniform) super;
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);

  if (inf > maxDist)
    return res;

  const vec3f localDir = self->world2light * dir;

  const float u = atan2(localDir.y, localDir.x) * one_over_two_pi;
  const float v = acos(localDir.z) * one_over_pi;
  const vec2f uv = make_vec2f(u, v);

  DifferentialGeometry lookup;
  initDgFromTexCoord(lookup, uv);
  res.radiance = get3f(self->map, lookup) * self->super.radiance;

  // domain of Distribution2D is shifted by half a texel compared to texture
  // atan2 can get negative, shift can lead to values > 1.f: reproject to [0..1)
  const vec2f uvd = frac(uv + self->map->halfTexel);
  res.pdf = Distribution2D_pdf(self->distribution, uvd);
  res.pdf *= one_over_two_pi_sqr * rsqrt(1.f - sqr(localDir.z));

  return res;
}

// bin i represents the average contribution of (i-0.5, i+0.5) when we sample
// the texture bilinearly at i
// for i==0 we have a wrap-around, which is wanted for x (phi), but actually
// not for y (theta), because then light (importance) from the south-pole is
// leaking to the north-pole
// however, sin(theta) is zero then, thus we will never sample there
task unmasked void HDRILight_calcRowImportance(
    const HDRILight *uniform const self,
    float *uniform const importance,
    float *uniform const row_importance)
{
  const uniform int y = taskIndex;
  const uniform float fy = y * self->rcpSize.y;
  const uniform int width = self->map->size.x;
  const uniform float sinTheta = sin(fy * M_PI);
  foreach (x = 0 ... width) {
    const vec2f coord = make_vec2f(x * self->rcpSize.x, fy);
    // using bilinear filtering is indeed what we want
    DifferentialGeometry lookup;
    initDgFromTexCoord(lookup, coord);
    const vec3f col = get3f(self->map, lookup);
    importance[y * width + x] = sinTheta * luminance(col);
  }
  row_importance[y] = Distribution1D_create(width, importance + y * width);
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

//! Set the parameters of an ispc-side HDRILight object
export void HDRILight_set(
    void *uniform super, const uniform linear3f &light2world, void *uniform map)
{
  HDRILight *uniform self = (HDRILight * uniform) super;

  Distribution2D_destroy(self->distribution);
  self->distribution = NULL;

  if (map) {
    self->light2world = light2world;
    self->world2light = rcp(light2world);

    self->map = (uniform Texture2D * uniform) map;

    self->rcpSize = 1.f / self->map->sizef;
    const uniform int height = self->map->size.y;

    // calculate importance in parallel
    float *uniform cdf_x = uniform new float[self->map->size.x * height];
    float *uniform row_importance = uniform new float[height];
    launch[height] HDRILight_calcRowImportance(self, cdf_x, row_importance);
    sync;

    // create distribution
    self->distribution =
        Distribution2D_create(self->map->size, cdf_x, row_importance);
    // no delete[] (row_)importance: ownership was transferred to Distribution2D

    self->super.sample = HDRILight_sample;
    self->super.eval = HDRILight_eval;
  } else {
    self->super.sample = HDRILight_sample_dummy;
    self->super.eval = Light_eval;
  }
}

//! Create an ispc-side HDRILight object
export void *uniform HDRILight_create()
{
  HDRILight *uniform self = uniform new HDRILight;

  Light_Constructor(&self->super);
  self->super.sample = HDRILight_sample_dummy;
  self->distribution = NULL;

  HDRILight_set(self, make_LinearSpace3f_identity(), NULL);

  return self;
}

//! Destroy an ispc-side HDRILight object
export void HDRILight_destroy(void *uniform super)
{
  HDRILight *uniform self = (HDRILight * uniform) super;
  Distribution2D_destroy(self->distribution);
  delete self;
}
