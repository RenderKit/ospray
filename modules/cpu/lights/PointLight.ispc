// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "IntensityDistribution.ih"
#include "common/Instance.ih"
#include "common/Intersect.ih"
#include "common/Ray.ih"
#include "math/sampling.ih"
#include "rkcommon/math/LinearSpace.ih"
// c++ shared
#include "PointLightShared.h"

// Implementation
//////////////////////////////////////////////////////////////////////////////
OSPRAY_BEGIN_ISPC_NAMESPACE

inline void Transform(const PointLight *uniform self,
    const uniform affine3f &xfm,
    uniform PointLightDynamic &dyn)
{
  dyn.position = xfmPoint(xfm, self->pre.position);

  if (self->intensityDistribution.lid) {
    dyn.direction = normalize(xfmVector(xfm, self->pre.direction));
    dyn.c90 = normalize(cross(xfmVector(xfm, self->pre.c0), dyn.direction));
    dyn.c0 = cross(dyn.direction, dyn.c90);
  }
}

export void PointLight_Transform(
    const void *uniform self, const void *uniform xfm, void *uniform dyn)
{
  Transform((PointLight * uniform) self,
      *((affine3f * uniform) xfm),
      *((PointLightDynamic * uniform) dyn));
}

inline Light_SampleRes Sample(const PointLight *uniform self,
    const uniform PointLightDynamic &dyn,
    const DifferentialGeometry &dg,
    const vec2f &s)
{
  Light_SampleRes res;

  // extant light vector from the hit point
  const vec3f dir = dyn.position - dg.P;
  const float dist2 = dot(dir, dir);
  const float invdist = rsqrt(dist2);

  // normalized light vector
  res.dir = dir * invdist;
  res.dist = dist2 * invdist;

  res.pdf = inf; // per default we always take this sample

  const float sinTheta = self->radius * invdist;

  if ((self->radius > 0.f) & (sinTheta > 0.005f)) {
    // sample surface of sphere as seen by hit point -> cone of directions
    // for very small cones treat as point light, because float precision is not
    // good enough
    if (sinTheta < 1.f) {
      const float cosTheta = sqrt(1.f - sinTheta * sinTheta);
      const vec3f localDir = uniformSampleCone(cosTheta, s);
      res.dir = frame(res.dir) * localDir;
      res.pdf = uniformSampleConePDF(cosTheta);
      const float c = localDir.z;
      res.dist =
          c * res.dist - sqrt_safe(sqr(self->radius) - (1.f - c * c) * dist2);
      // note the sampling PDF is already in solid angle therefore, we do not
      // need to divide it by the squared distance
      res.weight = self->radiance / res.pdf;
    } else {
      // emit only from the surface to the outside
      res.weight = make_vec3f(0.f);
    }
  } else {
    // convert from intensity to radiance by attenuating by distance^2
    res.weight = self->intensity * sqr(invdist);
  }

  if (self->intensityDistribution.lid) {
    const float cosd = -dot(dyn.direction, res.dir);
    res.weight = res.weight
        * IntensityDistribution_eval(
            &self->intensityDistribution, dyn.c0, dyn.c90, cosd, res.dir);
  }

  return res;
}

SYCL_EXTERNAL Light_SampleRes PointLight_sample(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &s,
    const float,
    const uniform FeatureFlagsHandler &)
{
  const PointLight *uniform self = (PointLight * uniform) super;
  assert(self);
  return Sample(self, self->pre, dg, s);
}

#ifndef OSPRAY_TARGET_SYCL
SYCL_EXTERNAL Light_SampleRes PointLight_sample_instanced(
    const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec2f &s,
    const float time,
    const uniform FeatureFlagsHandler &)
{
  const PointLight *uniform self = (PointLight * uniform) super;
  assert(self);

  const Instance *uniform instance = self->super.instance;
  assert(instance);

  Light_SampleRes res;
  foreach_unique (utime in time) {
    const uniform affine3f xfm = Instance_getTransform(instance, utime);
    uniform PointLightDynamic dyn;
    Transform(self, xfm, dyn);
    res = Sample(self, dyn, dg, s);
  }
  return res;
}
#endif

inline Light_EvalRes Eval(const PointLight *uniform self,
    const uniform PointLightDynamic &dyn,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist)
{
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);

  const vec3f A = dyn.position - dg.P;
  const float centerDist2 = dot(A, A);
  const float sinTheta2 = sqr(self->radius) * rcp(centerDist2);

  if (self->radius > 0.f && sqrt(sinTheta2) > 0.005f) {
    const Intersections isect =
        intersectSphere(dg.P, dir, dyn.position, self->radius);

    if (isect.entry.hit && isect.entry.t > minDist && isect.entry.t < maxDist) {
      const float cosTheta = sqrt(1.f - sinTheta2);
      res.pdf = uniformSampleConePDF(cosTheta);
      res.radiance = self->radiance;
      if (self->intensityDistribution.lid) {
        const float cosAngle = -dot(dyn.direction, dir);
        res.radiance = res.radiance
            * IntensityDistribution_eval(
                &self->intensityDistribution, dyn.c0, dyn.c90, cosAngle, dir);
      }
    }
  }
  return res;
}

SYCL_EXTERNAL Light_EvalRes PointLight_eval(const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist,
    const float)
{
  const PointLight *uniform self = (PointLight * uniform) super;
  assert(self);
  return Eval(self, self->pre, dg, dir, minDist, maxDist);
}

#ifndef OSPRAY_TARGET_SYCL
SYCL_EXTERNAL Light_EvalRes PointLight_eval_instanced(
    const Light *uniform super,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist,
    const float time)
{
  const PointLight *uniform self = (PointLight * uniform) super;
  assert(self);

  const Instance *uniform instance = self->super.instance;
  assert(instance);

  Light_EvalRes res;
  foreach_unique (utime in time) {
    const uniform affine3f xfm = Instance_getTransform(instance, utime);
    uniform PointLightDynamic dyn;
    Transform(self, xfm, dyn);
    res = Eval(self, dyn, dg, dir, minDist, maxDist);
  }
  return res;
}
#endif

// Exports (called from C++)
//////////////////////////////////////////////////////////////////////////////

export void *uniform PointLight_sample_addr()
{
  return (void *uniform)PointLight_sample;
}

#ifndef OSPRAY_TARGET_SYCL
export void *uniform PointLight_sample_instanced_addr()
{
  return (void *uniform)PointLight_sample_instanced;
}
#endif

export void *uniform PointLight_eval_addr()
{
  return (void *uniform)PointLight_eval;
}

#ifndef OSPRAY_TARGET_SYCL
export void *uniform PointLight_eval_instanced_addr()
{
  return (void *uniform)PointLight_eval_instanced;
}
#endif
OSPRAY_END_ISPC_NAMESPACE
