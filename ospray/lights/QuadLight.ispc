// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.ih"

struct QuadLight
{
  Light super; //!< inherited light fields

  vec3f position; //!< world-space corner position of the light
  vec3f edge1; //!< vectors to adjacent corners
  vec3f edge2; //!< vectors to adjacent corners

  vec3f nnormal; //!< negated normal, the direction that the QuadLight is not
                 //!< emitting; normalized
  float ppdf; // probability to sample point on light = 1/area
};

// Implementation
//////////////////////////////////////////////////////////////////////////////

Light_SampleRes QuadLight_sample(const uniform Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &sp)
{
  const QuadLight *uniform self = (QuadLight * uniform) super;
  Light_SampleRes res;

  // (0, 0) should sample center --> add 0.5 CranleyPatterson rotation
  vec2f s;
  s.x = sp.x > 0.5f ? sp.x - 0.5f : sp.x + 0.5f;
  s.y = sp.y > 0.5f ? sp.y - 0.5f : sp.y + 0.5f;

  // sample position on light with density ppdf = 1/area
  // TODO: use solid angle sampling
  const vec3f pos = self->position + self->edge1 * s.x + self->edge2 * s.y;

  // extant light vector from the hit point
  const vec3f dir = pos - dg.P;
  const float dist = length(dir);

  // normalized light vector
  res.dir = dir / dist;
  res.dist = dist;

  // convert to pdf wrt. solid angle
  const float cosd = dot(self->nnormal, res.dir);
  res.pdf = self->ppdf * sqr(dist) / abs(cosd);

  // emit only to one side
  res.weight =
      cosd > 0.f ? self->super.radiance * rcp(res.pdf) : make_vec3f(0.f);

  return res;
}

Light_EvalRes QuadLight_eval(const uniform Light *uniform super,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist)
{
  uniform QuadLight *uniform self = (uniform QuadLight * uniform) super;
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);

  // backfacing?
  const float cosd = dot(self->nnormal, dir);
  // denominator = dot(cross(edge1, edge2), dir) == cosd/ppdf
  if (cosd <= 0.f)
    return res;

  const vec3f c = self->position - dg.P;
  const vec3f r = cross(c, dir);
  const float u = dot(r, self->edge1);
  const float v = -dot(r, self->edge2);

  // u/denominator > 1?
  if (min(u, v) < 0.f | max(u, v) * self->ppdf > cosd)
    return res;

  const float rcosd = rcp(cosd);
  const float dist = dot(self->nnormal, c) * rcosd;
  if (dist <= minDist | dist > maxDist)
    return res;

  res.radiance = self->super.radiance;
  res.pdf = self->ppdf * sqr(dist) * rcosd;

  return res;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

//! Set the parameters of an ispc-side QuadLight object
export void QuadLight_set(void *uniform super,
    const uniform vec3f &position,
    const uniform vec3f &edge1,
    const uniform vec3f &edge2)
{
  uniform QuadLight *uniform self = (uniform QuadLight * uniform) super;
  self->position = position;
  self->edge1 = edge1;
  self->edge2 = edge2;

  const uniform vec3f ndirection = cross(edge2, edge1);
  self->ppdf = rcp(length(ndirection)); // 1/area
  self->nnormal = ndirection * self->ppdf; // normalize
}

//! Create an ispc-side QuadLight object
export void *uniform QuadLight_create()
{
  uniform QuadLight *uniform self = uniform new uniform QuadLight;

  Light_Constructor(&self->super);
  self->super.sample = QuadLight_sample;
  self->super.eval = QuadLight_eval;

  QuadLight_set(self,
      make_vec3f(0.f),
      make_vec3f(1.f, 0.f, 0.f),
      make_vec3f(0.f, 1.f, 0.f));

  return self;
}
