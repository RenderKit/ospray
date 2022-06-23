// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "IntensityDistribution.ih"
#include "Light.ih"
#include "SphericalQuadSampling.ih"
#include "common/Instance.ih"

// state that changes under transformations
struct QuadLightDynamic
{
  vec3f position;
  vec3f edge1;
  vec3f edge2;
  vec3f nnormal; // negated normal, the direction that the QuadLight is not
                 // emitting; normalized
  float ppdf; // probability to sample point on light = 1/area
  vec3f c0; // orientation for intensityDistribution: direction of the C0-
  vec3f c90; // and the C90-(half)plane
};

struct QuadLight
{
  Light super;
  vec3f radiance; // emitted RGB radiance
  IntensityDistribution intensityDistribution;
  QuadLightDynamic pre; // un- or pre-transformed state
};

// Implementation
//////////////////////////////////////////////////////////////////////////////
inline void Transform(const QuadLight *uniform self,
    const uniform affine3f xfm,
    uniform QuadLightDynamic &dyn)
{
  // transform light into the world space
  dyn.position = xfmPoint(xfm, self->pre.position);
  dyn.edge1 = xfmVector(xfm, self->pre.edge1);
  dyn.edge2 = xfmVector(xfm, self->pre.edge2);

  // calculate quad normal vector
  const uniform vec3f ndirection = cross(dyn.edge2, dyn.edge1);
  dyn.ppdf = rcp(length(ndirection)); // 1/area
  dyn.nnormal = ndirection * dyn.ppdf; // normalize

  if (self->intensityDistribution.lid) {
    dyn.c90 = normalize(cross(dyn.nnormal, xfmVector(xfm, self->pre.c0)));
    dyn.c0 = cross(dyn.c90, dyn.nnormal);
  }
}

inline Light_SampleRes SampleArea(const QuadLight *uniform self,
    const uniform QuadLightDynamic &dyn,
    const DifferentialGeometry &dg,
    const vec2f &s)
{
  Light_SampleRes res;
  const vec3f p = dyn.position + dyn.edge1 * s.x + dyn.edge2 * s.y;

  // extant light vector from the hit point
  const vec3f dir = p - dg.P;
  const float dist = length(dir);

  // normalized light vector
  res.dir = dir / dist;
  res.dist = dist;

  // convert to pdf wrt. solid angle
  const float cosd = dot(dyn.nnormal, res.dir);
  res.pdf = dyn.ppdf * sqr(dist) / abs(cosd);

  float weight;
  if (self->intensityDistribution.lid) {
    weight = IntensityDistribution_eval(
        &self->intensityDistribution, dyn.c0, dyn.c90, cosd, res.dir);
    // when an light distribution function is used we want to
    // remove the cosine term. To avoid numerical issues
    // at cosineAngle = 0 we use the fact that the division
    // of radiance with the cosine cancels out.
    weight /= dyn.ppdf * sqr(dist);
  } else {
    // emit only to one side
    weight = cosd > 0.f ? rcp(res.pdf) : 0.f;
  }
  res.weight = self->radiance * weight;

  return res;
}

inline Light_SampleRes Sample(const QuadLight *uniform self,
    const uniform QuadLightDynamic &dyn,
    const DifferentialGeometry &dg,
    const vec2f &s)
{
  Light_SampleRes res;

  if (s.x == 0.0f && s.y == 0.0f) { // XXX SciVis
    vec2f ss = make_vec2f(0.5f);
    return SampleArea(self, dyn, dg, ss);
  }

  // create spherical quad for solid angle sampling
  SphericalQuad quad = SphericalQuad_create(
      dyn.position, dyn.edge1, dyn.edge2, neg(dyn.nnormal), dg.P);

  // bilinear warped cosine weight approximation
  const vec3f cosW = computeCosineWeightedRNG(
      dyn.position, dyn.edge1, dyn.edge2, dg.P, dg.Ng, s);

  // sample quad
  const vec3f dir = sampleSphericalQuad(quad, make_vec2f(cosW.x, cosW.y));
  const float dist = length(dir);

  // normalized light vector
  res.dir = dir / dist;
  res.dist = dist;

  // convert to pdf wrt. solid angle
  const float cosd = dot(dyn.nnormal, res.dir);
  res.pdf = quad.S == 0.f ? 0.f : cosW.z / quad.S;

  if (self->intensityDistribution.lid) {
    if (abs(cosd) < 0.005f) // handle numerical edge case
      return SampleArea(self, dyn, dg, s);
    else {
      float weight = IntensityDistribution_eval(
          &self->intensityDistribution, dyn.c0, dyn.c90, cosd, res.dir);
      // when an light distribution function is used we want to
      // remove the cosine term. To avoid numerical issues
      // at cosineAngle = 0 we use the fact that the division
      // of radiance with the cosine cancels out.
      weight /= abs(cosd) * res.pdf;
      res.weight = res.pdf != 0.f ? self->radiance * weight : make_vec3f(0.f);
    }
  } else {
    // emit only to one side
    res.weight = (cosd > 0.f) && (res.pdf != 0.f)
        ? self->radiance * rcp(res.pdf)
        : make_vec3f(0.f);
  }

  return res;
}

Light_SampleRes QuadLight_sample(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &sp,
    const float)
{
  const QuadLight *uniform self = (QuadLight * uniform) super;
  assert(self);
  return Sample(self, self->pre, dg, sp);
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
    uniform QuadLightDynamic dyn;
    Transform(self, xfm, dyn);
    res = Sample(self, dyn, dg, sp);
  }
  return res;
}

inline Light_EvalRes Eval(const QuadLight *uniform self,
    const uniform QuadLightDynamic &dyn,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist)
{
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);

  // backfacing?
  const float cosd = dot(dyn.nnormal, dir);
  // denominator = dot(cross(edge1, edge2), dir) == cosd/ppdf
  if (cosd <= 0.f && !self->intensityDistribution.lid)
    return res;

  const vec3f c = dyn.position - dg.P;
  const vec3f r = cross(c, dir);
  const float rcosd = rcp(cosd);
  const float u = dot(r, dyn.edge1) * rcosd;
  const float v = -dot(r, dyn.edge2) * rcosd;

  // u/denominator > 1?
  if (min(u, v) < 0.f | max(u, v) * dyn.ppdf > 1.0f)
    return res;

  const float dist = dot(dyn.nnormal, c) * rcosd;
  if (dist <= minDist | dist > maxDist)
    return res;

  SphericalQuad quad = SphericalQuad_create(
      dyn.position, dyn.edge1, dyn.edge2, neg(dyn.nnormal), dg.P);
  if (quad.S > 0.f) {
    res.radiance = self->radiance;
    if (self->intensityDistribution.lid) {
      // convert from intensity to radiance by canceling the the cosine
      // term introduced by the Lambertian area light
      res.radiance = res.radiance
          * (IntensityDistribution_eval(
                 &self->intensityDistribution, dyn.c0, dyn.c90, cosd, dir)
              / abs(cosd));
    }
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
  return Eval(self, self->pre, dg, dir, minDist, maxDist);
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
    uniform QuadLightDynamic dyn;
    Transform(self, xfm, dyn);
    res = Eval(self, dyn, dg, dir, minDist, maxDist);
  }
  return res;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void QuadLight_set(void *uniform super,
    const uniform vec3f &radiance,
    const uniform vec3f &position,
    const uniform vec3f &edge1,
    const uniform vec3f &edge2,
    float *uniform lid,
    const uniform vec2i &size,
    const uniform vec3f &c0)
{
  QuadLight *uniform self = (QuadLight * uniform) super;
  self->radiance = radiance;

  self->pre.position = position;
  self->pre.edge1 = edge1;
  self->pre.edge2 = edge2;

  IntensityDistribution_set(&self->intensityDistribution, lid, size);

  // Enable dynamic runtime instancing or apply static transformation
  const Instance *uniform instance = self->super.instance;
  if (instance) {
    self->pre.c0 = c0;
    if (instance->motionBlur) {
      self->super.sample = QuadLight_sample_instanced;
      self->super.eval = QuadLight_eval_instanced;
    } else
      Transform(self, instance->xfm, self->pre);
  } else {
    const uniform vec3f ndirection = cross(edge2, edge1);
    self->pre.ppdf = rcp(length(ndirection)); // 1/area
    self->pre.nnormal = ndirection * self->pre.ppdf; // normalize
    self->pre.c90 = normalize(cross(self->pre.nnormal, c0));
    self->pre.c0 = cross(self->pre.c90, self->pre.nnormal);
  }
}

//! Create an ispc-side QuadLight object
export void *uniform QuadLight_create()
{
  QuadLight *uniform self = uniform new QuadLight;

  Light_Constructor(&self->super);
  self->super.sample = QuadLight_sample;
  self->super.eval = QuadLight_eval;

  QuadLight_set(self,
      make_vec3f(0.f),
      make_vec3f(0.f),
      make_vec3f(1.f, 0.f, 0.f),
      make_vec3f(0.f, 1.f, 0.f),
      NULL,
      make_vec2i(0),
      make_vec3f(0.f, 1.f, 0.f));

  return self;
}
