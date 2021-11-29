// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.ih"
#include "SphericalQuadSampling.ih"
#include "common/Instance.ih"

struct QuadLight
{
  Light super; //!< inherited light fields

  vec3f radiance; //!< emmitted RGB radiance
  vec3f position; //!< world-space corner position of the light
  vec3f edge1; //!< vectors to adjacent corners
  vec3f edge2; //!< vectors to adjacent corners

  vec3f nnormal; //!< negated normal, the direction that the QuadLight is not
                 //!< emitting; normalized
  float ppdf; // probability to sample point on light = 1/area
};

// Implementation
//////////////////////////////////////////////////////////////////////////////

inline Light_SampleRes Sample(const QuadLight *uniform self,
    const uniform vec3f &pos,
    const uniform vec3f &edge1,
    const uniform vec3f &edge2,
    const uniform float ppdf,
    const uniform vec3f &nnormal,
    const DifferentialGeometry &dg,
    const vec2f &sp)
{
  Light_SampleRes res;

  // (0, 0) should sample center --> add 0.5 CranleyPatterson rotation
  vec2f s;
  s.x = sp.x > 0.5f ? sp.x - 0.5f : sp.x + 0.5f;
  s.y = sp.y > 0.5f ? sp.y - 0.5f : sp.y + 0.5f;

  // create spherical quad for solid angle sampling
  SphericalQuad quad =
      SphericalQuad_create(pos, edge1, edge2, neg(nnormal), dg.P);

  // bilinear warped cosine weight approximation
  const vec3f cosW =
      computeCosineWeightedRNG(pos, edge1, edge2, dg.P, dg.Ng, s);

  // sample quad
  const vec3f dir = sampleSphericalQuad(quad, make_vec2f(cosW.x, cosW.y));
  const float dist = length(dir);

  // normalized light vector
  res.dir = dir / dist;
  res.dist = dist;

  // convert to pdf wrt. solid angle
  const float cosd = dot(nnormal, res.dir);
  res.pdf = quad.S == 0.f ? 0.f : cosW.z / quad.S;

  // emit only to one side
  res.weight = (cosd > 0.f) && (res.pdf != 0.f) ? self->radiance * rcp(res.pdf)
                                                : make_vec3f(0.f);

  return res;
}

Light_SampleRes QuadLight_sample(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &sp,
    const float)
{
  const QuadLight *uniform self = (QuadLight * uniform) super;
  assert(self);
  return Sample(self,
      self->position,
      self->edge1,
      self->edge2,
      self->ppdf,
      self->nnormal,
      dg,
      sp);
}

Light_SampleRes QuadLight_sample_instanced(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &sp,
    const float time)
{
  const QuadLight *uniform self = (QuadLight * uniform) super;
  assert(self);

  const Instance *uniform instance = self->super.instance;
  assert(instance);

  Light_SampleRes res;
  foreach_unique (utime in time) {
    const uniform affine3f xfm = Instance_getTransform(instance, utime);

    // transform light into the world space
    const uniform vec3f pos = xfmPoint(xfm, self->position);
    const uniform vec3f edge1 = xfmVector(xfm, self->edge1);
    const uniform vec3f edge2 = xfmVector(xfm, self->edge2);

    // calculate quad normal vector
    const uniform vec3f ndirection = cross(edge2, edge1);
    const uniform float ppdf = rcp(length(ndirection)); // 1/area
    const uniform vec3f nnormal = ndirection * ppdf; // normalize

    // sample light
    res = Sample(self, pos, edge1, edge2, ppdf, nnormal, dg, sp);
  }
  return res;
}

inline Light_EvalRes Eval(const QuadLight *uniform self,
    const uniform vec3f &pos,
    const uniform vec3f &edge1,
    const uniform vec3f &edge2,
    const uniform float ppdf,
    const uniform vec3f &nnormal,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist)
{
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);

  // backfacing?
  const float cosd = dot(nnormal, dir);
  // denominator = dot(cross(edge1, edge2), dir) == cosd/ppdf
  if (cosd <= 0.f)
    return res;

  const vec3f c = pos - dg.P;
  const vec3f r = cross(c, dir);
  const float u = dot(r, edge1);
  const float v = -dot(r, edge2);

  // u/denominator > 1?
  if (min(u, v) < 0.f | max(u, v) * ppdf > cosd)
    return res;

  const float rcosd = rcp(cosd);
  const float dist = dot(nnormal, c) * rcosd;
  if (dist <= minDist | dist > maxDist)
    return res;

  SphericalQuad quad =
      SphericalQuad_create(pos, edge1, edge2, neg(nnormal), dg.P);
  if (quad.S > 0.f) {
    res.radiance = self->radiance;
    res.pdf = rcp(quad.S);
  }

  return res;
}

Light_EvalRes QuadLight_eval(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist,
    const float)
{
  const QuadLight *uniform self = (QuadLight * uniform) super;
  assert(self);
  return Eval(self,
      self->position,
      self->edge1,
      self->edge2,
      self->ppdf,
      self->nnormal,
      dg,
      dir,
      minDist,
      maxDist);
}

Light_EvalRes QuadLight_eval_instanced(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist,
    const float time)
{
  const QuadLight *uniform self = (QuadLight * uniform) super;
  assert(self);

  const Instance *uniform instance = self->super.instance;
  assert(instance);

  Light_EvalRes res;
  foreach_unique (utime in time) {
    const uniform affine3f xfm = Instance_getTransform(instance, utime);

    // transform light into the world space
    const uniform vec3f pos = xfmPoint(xfm, self->position);
    const uniform vec3f edge1 = xfmVector(xfm, self->edge1);
    const uniform vec3f edge2 = xfmVector(xfm, self->edge2);

    // calculate quad normal vector
    const uniform vec3f ndirection = cross(edge2, edge1);
    const uniform float ppdf = rcp(length(ndirection)); // 1/area
    const uniform vec3f nnormal = ndirection * ppdf; // normalize

    // evaluate light
    res =
        Eval(self, pos, edge1, edge2, ppdf, nnormal, dg, dir, minDist, maxDist);
  }
  return res;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

//! Set the parameters of an ispc-side QuadLight object
export void QuadLight_set(void *uniform super,
    const uniform vec3f &radiance,
    const uniform vec3f &position,
    const uniform vec3f &edge1,
    const uniform vec3f &edge2)
{
  uniform QuadLight *uniform self = (uniform QuadLight * uniform) super;
  self->radiance = radiance;
  self->position = position;
  self->edge1 = edge1;
  self->edge2 = edge2;

  // Enable dynamic runtime instancing or apply static transformation
  const Instance *uniform instance = self->super.instance;
  if (instance) {
    if (instance->motionBlur) {
      self->super.sample = QuadLight_sample_instanced;
      self->super.eval = QuadLight_eval_instanced;
    } else {
      self->position = xfmPoint(instance->xfm, self->position);
      self->edge1 = xfmVector(instance->xfm, self->edge1);
      self->edge2 = xfmVector(instance->xfm, self->edge2);
    }
  }

  const uniform vec3f ndirection = cross(self->edge2, self->edge1);
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
      make_vec3f(0.f),
      make_vec3f(1.f, 0.f, 0.f),
      make_vec3f(0.f, 1.f, 0.f));

  return self;
}
