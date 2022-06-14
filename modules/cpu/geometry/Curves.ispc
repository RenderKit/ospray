// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Geometry.ih"
// c++ shared
#include "CurvesShared.h"

static void Curves_postIntersect(const Geometry *uniform _self,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  Curves *uniform self = (Curves * uniform) _self;
  dg.Ng = dg.Ns = ray.Ng;
  flags &= self->flagMask;
  if (flags & DG_COLOR) {
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
  }
  if (flags & DG_TEXCOORD) {
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
  }
}

export void *uniform Curves_postIntersect_addr()
{
  return Curves_postIntersect;
}