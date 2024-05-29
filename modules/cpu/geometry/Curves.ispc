// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Geometry.ih"
#include "common/Data.ih"
#include "common/FeatureFlags.ih"
// c++ shared
#include "CurvesShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

#ifdef OSPRAY_TARGET_SYCL
using namespace ospray;
#endif

template <typename T>
inline T interpolate(const Curves *uniform self,
    const uniform Data1D &buffer,
    const varying Ray &ray,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsGeometry ffg = getFeatureFlagsGeometry(ffh);
  const uint32 index = get<uint32>(self->index, ray.primID);
  const float t = ray.u;

  const T v0 = get<T>(buffer, index + 0);
  const T v1 = get<T>(buffer, index + 1);

  if (((self->curveBasis == OSP_LINEAR) && (ffg & FFG_LINEAR_CURVE))
      || ((self->curveBasis == OSP_HERMITE) && (ffg & FFG_HERMITE_CURVE))) {
    return lerp(t, v0, v1);
  }

  if ((ffg & FFG_BEZIER_CURVE) || (ffg & FFG_BSPLINE_CURVE)
      || (ffg & FFG_CATMULL_ROM_CURVE)) {
    const T v2 = get<T>(buffer, index + 2);
    const T v3 = get<T>(buffer, index + 3);
    const float s = 1.0f - ray.u;
    const float s2 = s * s;
    const float t2 = t * t;
    const float s2t = s2 * t;
    const float t2s = t2 * s;
    const float s3 = s2 * s;
    const float t3 = t2 * t;

    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f;
    float c = 0.0f;

    if ((self->curveBasis == OSP_BEZIER) && (ffg & FFG_BEZIER_CURVE)) {
      b0 = s3;
      b1 = 3.0f * s2t;
      b2 = 3.0f * t2s;
      b3 = t3;
      c = 1.0f;
    }

    if ((self->curveBasis == OSP_BSPLINE) && (ffg & FFG_BSPLINE_CURVE)) {
      b0 = s3;
      b1 = (4.0f * s3 + t3) + (12.0f * s2t + 6.0f * t2s);
      b2 = (4.0f * t3 + s3) + (12.0f * t2s + 6.0f * s2t);
      b3 = t3;
      c = 1.0f / 6.0f;
    }

    if ((self->curveBasis == OSP_CATMULL_ROM)
        && (ffg & FFG_CATMULL_ROM_CURVE)) {
      b0 = -s2t;
      b1 = 2.0f + t2 * (3.0f * t - 5.0f);
      b2 = 2.0f + s2 * (3.0f * s - 5.0f);
      b3 = -t2s;
      c = 0.5f;
    }
    return c * (b0 * v0 + b1 * v1 + b2 * v2 + b3 * v3);
  }

  T result = {0};
  return result;
}

SYCL_EXTERNAL void Curves_postIntersect(const Geometry *uniform _self,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags,
    const uniform FeatureFlagsHandler &ffh)
{
  const Curves *uniform self = (const Curves *uniform)_self;

  dg.Ng = dg.Ns = ray.Ng;
  flags &= self->flagMask;

  if (flags & DG_COLOR) {
    dg.color = interpolate<vec4f>(self, self->color, ray, ffh);
  }

  if (flags & DG_TEXCOORD) {
    dg.st = interpolate<vec2f>(self, self->texcoord, ray, ffh);
  }
}

export void *uniform Curves_postIntersect_addr()
{
  return (void *uniform)Curves_postIntersect;
}

OSPRAY_END_ISPC_NAMESPACE
