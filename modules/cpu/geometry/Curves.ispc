// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Geometry.ih"
#include "common/Data.ih"
// c++ shared
#include "CurvesShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

#ifdef OSPRAY_TARGET_SYCL
using namespace ospray;

template <typename T>
inline T interpolate(const Curves *self,
    const Data1D &buffer,
    const uint32 primID,
    const float u)
{
  T result;
  const uint32 index = get<uint32>(self->index, primID);

  // Handle all curve basis
  switch (self->curveBasis) {
  case OSP_LINEAR:
  case OSP_HERMITE: {
    const T v0 = get<T>(buffer, index + 0);
    const T v1 = get<T>(buffer, index + 1);
    result = lerp(u, v0, v1);
  } break;

  case OSP_BEZIER: {
    const T v0 = get<T>(buffer, index + 0);
    const T v1 = get<T>(buffer, index + 1);
    const T v2 = get<T>(buffer, index + 2);
    const T v3 = get<T>(buffer, index + 3);
    const float t1 = u;
    const float t0 = 1.0f - t1;
    const float b0 = t0 * t0 * t0;
    const float b1 = 3.0f * t1 * (t0 * t0);
    const float b2 = 3.0f * (t1 * t1) * t0;
    const float b3 = t1 * t1 * t1;
    result = T(b0 * v0 + b1 * v1 + b2 * v2 + b3 * v3);
  } break;

  case OSP_BSPLINE: {
    const T v0 = get<T>(buffer, index + 0);
    const T v1 = get<T>(buffer, index + 1);
    const T v2 = get<T>(buffer, index + 2);
    const T v3 = get<T>(buffer, index + 3);
    const float t = u;
    const float s = 1.0f - u;
    const float n0 = s * s * s;
    const float n1 = (4.0f * (s * s * s) + (t * t * t))
        + (12.0f * ((s * t) * s) + 6.0f * ((t * s) * t));
    const float n2 = (4.0f * (t * t * t) + (s * s * s))
        + (12.0f * ((t * s) * t) + 6.0f * ((s * t) * s));
    const float n3 = t * t * t;
    result = T((1.0f / 6.0f) * (n0 * v0 + n1 * v1 + n2 * v2 + n3 * v3));
  } break;

  case OSP_CATMULL_ROM: {
    const T v0 = get<T>(buffer, index + 0);
    const T v1 = get<T>(buffer, index + 1);
    const T v2 = get<T>(buffer, index + 2);
    const T v3 = get<T>(buffer, index + 3);
    const float t = u;
    const float s = 1.0f - u;
    const float n0 = -t * s * s;
    const float n1 = 2.0f + t * t * (3.0f * t - 5.0f);
    const float n2 = 2.0f + s * s * (3.0f * s - 5.0f);
    const float n3 = -s * t * t;
    result = T(0.5f * (n0 * v0 + n1 * v1 + n2 * v2 + n3 * v3));
  } break;

  case OSP_UNKNOWN_CURVE_BASIS:
  default:
    result = T(0.0f);
  }

  // Return interpolated value
  return result;
}
#endif

SYCL_EXTERNAL void Curves_postIntersect(const Geometry *uniform _self,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  Curves *uniform self = (Curves * uniform) _self;
  dg.Ng = dg.Ns = ray.Ng;
  flags &= self->flagMask;
  if (flags & DG_COLOR) {
#ifndef OSPRAY_TARGET_SYCL
    rtcInterpolateV1(self->geom,
        ray.primID,
        ray.u,
        ray.v,
        RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        0,
        &dg.color.x,
        NULL,
        NULL,
        4);
#else
    dg.color = interpolate<vec4f>(self, self->color, ray.primID, ray.u);
#endif
  }
  if (flags & DG_TEXCOORD) {
#ifndef OSPRAY_TARGET_SYCL
    rtcInterpolateV1(self->geom,
        ray.primID,
        ray.u,
        ray.v,
        RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        1,
        &dg.st.x,
        NULL,
        NULL,
        2);
#else
    dg.st = interpolate<vec2f>(self, self->texcoord, ray.primID, ray.u);
#endif
  }
}

export void *uniform Curves_postIntersect_addr()
{
  return (void *uniform)Curves_postIntersect;
}

OSPRAY_END_ISPC_NAMESPACE
