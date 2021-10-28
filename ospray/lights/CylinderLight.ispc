// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../math/AffineSpace.ih"
#include "CylinderCapSolidAngleTable.ih"
#include "Light.ih"
#include "SphericalQuadSampling.ih"
#include "common/Instance.ih"
#include "common/Intersect.ih"

struct CylinderLight
{
  Light super;

  vec3f position0;
  vec3f position1;
  vec3f radiance;
  float radius;
};

// Implementation
//////////////////////////////////////////////////////////////////////////////
inline float cylinderAxialSolidAngle(
    float c, float b, float h0, float h1, float b2c2)
{
  return 2.f * atan(c * h1 / (b * sqrt(b2c2 + sqr(h1))))
      - 2.f * atan(c * h0 / (b * sqrt(b2c2 + sqr(h0))));
}

inline float cylinderCapSolidAngle(float d, float R, float h)
{
  const float dmR = d - R;
  const float dpR = d + R;
  const float dmR2 = dmR * dmR;
  const float R2 = R * R;
  const float h2 = h * h;

  const float u = sqrt(dmR / dpR);
  const float v = sqrt((dmR2 + h2) / (d * d - R2 + h2));
  const float w = sqrt((dmR2 + h2) / (dpR * dpR + h2));

  const float integral = sampleCylSolidAngleTable(u, v, w);

  const float recip = sqrt(dmR2 + h2);
  return (16.f * R2 * h / (recip * recip * recip)) * integral;
}

inline void cylinderComputeRectBounds(const uniform vec3f &position0,
    const uniform vec3f &position1,
    const uniform float radius,
    const vec3f &P,
    vec3f &q0,
    vec3f &e0,
    vec3f &e1,
    float &S)
{
  // construct geometry of cylinder from sample point
  vec3f O = position0;
  vec3f Z = position1 - position0;
  const float H = length(Z);
  Z = Z / H;
  vec3f Dp = P - O;
  float h0 = -dot(Dp, Z);

  // flip origin if end of cylinder is closer to sample point
  if (h0 < -H) {
    O = position1;
    Z = -1.f * Z;
    Dp = P - O;
    h0 = -dot(Dp, Z);
  }
  const vec3f D = -h0 * Z - Dp;
  const float d = length(D);
  const float srad = min(radius / d, 1.f);
  const float q = max(1.f - sqr(srad), 4.e-6f);
  const float b = d * q;
  const float c = radius * sqrt(q);
  const float b2c2 = sqr(b) + sqr(c);
  const vec3f X = normalize(cross(D, Z));

  const float h1 = (1.f + srad) * (h0 + H);

  // compute the bounding rectangle of the cylinder
  const float hO = h0 < 0.f ? (1.f + srad) * h0 : h0;
  q0 = P + D * q - c * X + hO * Z;
  e0 = 2.f * c * X;
  e1 = (h1 - hO) * Z;

  // compute solid angle of cylinder at sample point
  S = cylinderAxialSolidAngle(c, b, hO, h1, b2c2);
}

inline void cylinderSampleSolidAngle(const DifferentialGeometry &dg,
    const vec2f &sp,
    const uniform vec3f &position0,
    const uniform vec3f &position1,
    const uniform float radius,
    vec3f &dir,
    float &pdf,
    float &S)
{
  // compute spherical quad bounding cylinder
  vec3f q0, e0, e1;
  cylinderComputeRectBounds(position0, position1, radius, dg.P, q0, e0, e1, S);
  const float e0l = length(e0);
  const float e1l = length(e1);
  const vec3f e0n = e0 / e0l;
  const vec3f e1n = e1 / e1l;
  const vec3f n = normalize(cross(e0n, e1n));
  SphericalQuad quad = SphericalQuad_create(q0, e0n, e0l, e1n, e1l, n, dg.P);

  // warped samples in solid angle space
  const vec3f cosW = computeCosineWeightedRNG(q0, e0, e1, dg.P, dg.Ng, sp);

  // sample spherical quad bounding cylinder
  dir = sampleSphericalQuad(quad, make_vec2f(cosW.x, cosW.y));
  pdf = cosW.z;
}

inline Light_SampleRes Sample(const DifferentialGeometry &dg,
    const vec2f &sp,
    const uniform vec3f &position0,
    const uniform vec3f &position1,
    const uniform float radius,
    const uniform vec3f &radiance)
{
  Light_SampleRes res;
  res.weight = make_vec3f(0.f);
  res.pdf = 0.f;

  float S, pdf;
  vec3f dir;
  if (sp.x == 0.0f && sp.y == 0.0f) { // XXX SciVis
    dir = 0.5f * (position0 + position1) - dg.P; // to center

    vec3f q0, e0, e1;
    cylinderComputeRectBounds(
        position0, position1, radius, dg.P, q0, e0, e1, S);
    if (S > 0.f) {
      pdf = 1.f;
    }
  } else {
    cylinderSampleSolidAngle(dg, sp, position0, position1, radius, dir, pdf, S);
  }

  if (S <= 0.f || pdf == 0.f) {
    return res;
  }

  res.dir = normalize(dir);

  // rejection sampling of the cylinder sample direction (through out cap hits
  // and rare misses)
  Intersections isect =
      intersectCylinder(dg.P, res.dir, position0, position1, radius);
  if (isect.entry.hit) {
    res.pdf = pdf / S;
    res.weight = radiance * rcp(res.pdf);
    res.dist = isect.entry.t;
  }

  return res;
}

Light_SampleRes CylinderLight_sample(const uniform Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &sp,
    const float)
{
  const CylinderLight *uniform self = (CylinderLight * uniform) super;
  assert(self);
  return Sample(
      dg, sp, self->position0, self->position1, self->radius, self->radiance);
}

Light_SampleRes CylinderLight_sample_instanced(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &sp,
    const float time)
{
  const CylinderLight *uniform self = (CylinderLight * uniform) super;
  assert(self);

  const Instance *uniform instance = self->super.instance;
  assert(instance);

  Light_SampleRes res;
  foreach_unique (utime in time) {
    const uniform affine3f xfm = Instance_getTransform(instance, utime);

    // transform light into the world space
    const uniform vec3f position0 = xfmPoint(xfm, self->position0);
    const uniform vec3f position1 = xfmPoint(xfm, self->position1);

    // sample light
    res = Sample(dg, sp, position0, position1, self->radius, self->radiance);
  }
  return res;
}

inline Light_EvalRes Eval(const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist,
    const uniform vec3f &position0,
    const uniform vec3f &position1,
    const uniform float radius,
    const uniform vec3f &radiance)
{
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);

  // check if intersecting cylinder
  Intersections isect =
      intersectCylinder(dg.P, dir, position0, position1, radius);
  if (isect.entry.hit) {
    if ((minDist < isect.entry.t && isect.entry.t <= maxDist)) {
      // compute bounding rectangle solid angle
      vec3f q0, e0, e1;
      float S;
      cylinderComputeRectBounds(
          position0, position1, radius, dg.P, q0, e0, e1, S);
      if (S > 0.f) {
        res.radiance = radiance;
        res.pdf = rcp(S);
      }
    }
  }

  return res;
}

Light_EvalRes CylinderLight_eval(const uniform Light *uniform super,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist,
    const float)
{
  uniform CylinderLight *uniform self = (uniform CylinderLight * uniform) super;
  assert(self);
  return Eval(dg,
      dir,
      minDist,
      maxDist,
      self->position0,
      self->position1,
      self->radius,
      self->radiance);
}

Light_EvalRes CylinderLight_eval_instanced(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist,
    const float time)
{
  const CylinderLight *uniform self = (CylinderLight * uniform) super;
  assert(self);

  const Instance *uniform instance = self->super.instance;
  assert(instance);

  Light_EvalRes res;
  foreach_unique (utime in time) {
    const uniform affine3f xfm = Instance_getTransform(instance, utime);

    // transform light into the world space
    const uniform vec3f position0 = xfmPoint(xfm, self->position0);
    const uniform vec3f position1 = xfmPoint(xfm, self->position1);

    // evaluate light
    res = Eval(dg,
        dir,
        minDist,
        maxDist,
        position0,
        position1,
        self->radius,
        self->radiance);
  }
  return res;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void CylinderLight_set(void *uniform super,
    const uniform vec3f &radiance,
    const uniform vec3f &position0,
    const uniform vec3f &position1,
    const uniform float radius)
{
  uniform CylinderLight *uniform self = (uniform CylinderLight * uniform) super;
  self->radiance = make_vec3f(0.f);
  uniform float zMax = length(position1 - position0);
  if (and(zMax > 0.f, radius > 0.f)) {
    self->radiance = radiance;
    self->radius = radius;
    self->position0 = position0;
    self->position1 = position1;
  }

  // Enable dynamic runtime instancing or apply static transformation
  const Instance *uniform instance = self->super.instance;
  if (instance) {
    if (instance->motionBlur) {
      self->super.sample = CylinderLight_sample_instanced;
      self->super.eval = CylinderLight_eval_instanced;
    } else {
      self->position0 = xfmPoint(instance->xfm, position0);
      self->position1 = xfmPoint(instance->xfm, position1);
    }
  }
}

export void *uniform CylinderLight_create()
{
  uniform CylinderLight *uniform self = uniform new uniform CylinderLight;

  Light_Constructor(&self->super);
  self->super.sample = CylinderLight_sample;
  self->super.eval = CylinderLight_eval;

  CylinderLight_set(
      self, make_vec3f(0.f), make_vec3f(0.f), make_vec3f(0.f, 0.f, 1.f), 1.f);

  return self;
}
