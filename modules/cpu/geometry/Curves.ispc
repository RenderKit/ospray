// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Geometry.ih"
#include "common/Data.ih"
// c++ shared
#include "CurvesShared.h"

#ifdef OSPRAY_TARGET_SYCL
using namespace ospray;
#endif

OSPRAY_BEGIN_ISPC_NAMESPACE

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
    // TODO: we can just do linear interpolation here,
    // need to exposed the interpolation methods from Embree's code
    const uint32 v0 = get_uint32(self->index, ray.primID);
    const uint32 v1 = get_uint32(self->index, ray.primID + 1);
    const vec4f color0 = get_vec4f(self->color, v0);
    const vec4f color1 = get_vec4f(self->color, v1);
    dg.color = lerp(ray.u, color0, color1);
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
    // TODO: we can just do linear interpolation here,
    // need to exposed the interpolation methods from Embree's code
    const uint32 v0 = get_uint32(self->index, ray.primID);
    const uint32 v1 = get_uint32(self->index, ray.primID + 1);
    const vec2f texcoord0 = get_vec2f(self->texcoord, v0);
    const vec2f texcoord1 = get_vec2f(self->texcoord, v1);
    dg.st = lerp(ray.u, texcoord0, texcoord1);
#endif
  }
}

export void *uniform Curves_postIntersect_addr()
{
  return (void *uniform)Curves_postIntersect;
}

OSPRAY_END_ISPC_NAMESPACE
