// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Geometry.ih"
#include "common/Data.ih"
// c++ shared
#include "SubdivisionShared.h"

static void Subdivision_postIntersect(const Geometry *uniform _self,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  Subdivision *uniform self = (Subdivision * uniform) _self;
  dg.Ng = dg.Ns = ray.Ng;

  flags &= self->flagMask;

  if (flags & DG_NS) {
    vec3f dPdu, dPdv;
    rtcInterpolateV1(self->geom,
        ray.primID,
        ray.u,
        ray.v,
        RTC_BUFFER_TYPE_VERTEX,
        0,
        NULL,
        &dPdu.x,
        &dPdv.x,
        3);
    dg.Ns = cross(dPdu, dPdv);
  }

  if (flags & DG_COLOR) {
    rtcInterpolateV0(self->geom,
        ray.primID,
        ray.u,
        ray.v,
        RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        0,
        &dg.color.x,
        4);
  }

  if (flags & DG_TEXCOORD) {
    rtcInterpolateV0(self->geom,
        ray.primID,
        ray.u,
        ray.v,
        RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        1,
        &dg.st.x,
        2);
  }
}

export void *uniform Subdivision_postIntersect_addr()
{
  return Subdivision_postIntersect;
}