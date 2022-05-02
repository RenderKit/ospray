// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../math/AffineSpace.ih"
#include "CylinderCapSolidAngleTable.ih"
#include "SphericalQuadSampling.ih"
#include "common/DifferentialGeometry.ih"
#include "common/Instance.ih"
#include "common/Intersect.ih"
// c++ shared
#include "CylinderLightShared.h"

// Implementation
//////////////////////////////////////////////////////////////////////////////
inline void Transform(const CylinderLight *uniform self,
    const uniform affine3f &xfm,
    uniform CylinderLightDynamic &dyn)
{
  dyn.position0 = xfmPoint(xfm, self->pre.position0);
  dyn.position1 = xfmPoint(xfm, self->pre.position1);
}

export void CylinderLight_Transform(
    const void *uniform self, const void *uniform xfm, void *uniform dyn)
{
  Transform((CylinderLight * uniform) self,
      *((affine3f * uniform) xfm),
      *((CylinderLightDynamic * uniform) dyn));
}

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
  const vec3f n = normalize(cross(e0, e1));
  SphericalQuad quad = SphericalQuad_create(q0, e0, e1, n, dg.P);

  // warped samples in solid angle space
  const vec3f cosW = computeCosineWeightedRNG(q0, e0, e1, dg.P, dg.Ng, sp);

  // sample spherical quad bounding cylinder
  dir = sampleSphericalQuad(quad, make_vec2f(cosW.x, cosW.y));
  pdf = cosW.z;
}

inline Light_SampleRes Sample(const CylinderLight *uniform self,
    const uniform CylinderLightDynamic &dyn,
    const DifferentialGeometry &dg,
    const vec2f &sp)
{
  Light_SampleRes res;
  res.weight = make_vec3f(0.f);
  res.pdf = 0.f;

  float S, pdf;
  vec3f dir;
  if (sp.x == 0.0f && sp.y == 0.0f) { // XXX SciVis
    dir = 0.5f * (dyn.position0 + dyn.position1) - dg.P; // to center

    vec3f q0, e0, e1;
    cylinderComputeRectBounds(
        dyn.position0, dyn.position1, self->radius, dg.P, q0, e0, e1, S);
    if (S > 0.f) {
      pdf = 1.f;
    }
  } else {
    cylinderSampleSolidAngle(
        dg, sp, dyn.position0, dyn.position1, self->radius, dir, pdf, S);
  }

  if (S <= 0.f || pdf == 0.f) {
    return res;
  }

  res.dir = normalize(dir);

  // rejection sampling of the cylinder sample direction (through out cap hits
  // and rare misses)
  Intersections isect = intersectCylinder(
      dg.P, res.dir, dyn.position0, dyn.position1, self->radius);
  if (isect.entry.hit) {
    res.pdf = pdf / S;
    res.weight = self->radiance * rcp(res.pdf);
    res.dist = isect.entry.t;
  }

  return res;
}

Light_SampleRes CylinderLight_sample(const uniform Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &s,
    const float)
{
  const CylinderLight *uniform self = (CylinderLight * uniform) super;
  assert(self);
  return Sample(self, self->pre, dg, s);
}

Light_SampleRes CylinderLight_sample_instanced(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &s,
    const float time)
{
  const CylinderLight *uniform self = (CylinderLight * uniform) super;
  assert(self);

  const Instance *uniform instance = self->super.instance;
  assert(instance);

  Light_SampleRes res;
  foreach_unique (utime in time) {
    const uniform affine3f xfm = Instance_getTransform(instance, utime);
    uniform CylinderLightDynamic dyn;
    Transform(self, xfm, dyn);
    res = Sample(self, dyn, dg, s);
  }
  return res;
}

inline Light_EvalRes Eval(const CylinderLight *uniform self,
    const uniform CylinderLightDynamic &dyn,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist)
{
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);

  // check if intersecting cylinder
  Intersections isect =
      intersectCylinder(dg.P, dir, dyn.position0, dyn.position1, self->radius);
  if (isect.entry.hit) {
    if ((minDist < isect.entry.t && isect.entry.t <= maxDist)) {
      // compute bounding rectangle solid angle
      vec3f q0, e0, e1;
      float S;
      cylinderComputeRectBounds(
          dyn.position0, dyn.position1, self->radius, dg.P, q0, e0, e1, S);
      if (S > 0.f) {
        res.radiance = self->radiance;
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
  CylinderLight *uniform self = (CylinderLight * uniform) super;
  assert(self);
  return Eval(self, self->pre, dg, dir, minDist, maxDist);
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
    uniform CylinderLightDynamic dyn;
    Transform(self, xfm, dyn);
    res = Eval(self, dyn, dg, dir, minDist, maxDist);
  }
  return res;
}

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void *uniform CylinderLight_sample_addr()
{
  return CylinderLight_sample;
}

export void *uniform CylinderLight_sample_instanced_addr()
{
  return CylinderLight_sample_instanced;
}

export void *uniform CylinderLight_eval_addr()
{
  return CylinderLight_eval;
}

export void *uniform CylinderLight_eval_instanced_addr()
{
  return CylinderLight_eval_instanced;
}
