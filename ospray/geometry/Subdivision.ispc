// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Geometry.ih"
#include "common/World.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"

struct Subdivision
{
  Geometry super;
  RTCGeometry geom;
  int64 flagMask; // which attributes are missing and cannot be interpolated
};

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

void Subdivision_Constructor(
    uniform Subdivision *uniform mesh, void *uniform cppEquivalent)
{
  Geometry_Constructor(&mesh->super, cppEquivalent, Subdivision_postIntersect);

  mesh->geom = NULL;
  mesh->flagMask = 0;
}

export void *uniform Subdivision_create(void *uniform cppEquivalent)
{
  Subdivision *uniform mesh = uniform new Subdivision;
  Subdivision_Constructor(mesh, cppEquivalent);
  return mesh;
}

export void *uniform Subdivision_set(void *uniform _mesh,
    RTCGeometry geom,
    uniform bool haveColor,
    uniform bool haveTC)
{
  Subdivision *uniform mesh = (Subdivision * uniform) _mesh;
  mesh->geom = geom;
  mesh->flagMask = -1;
  if (!haveColor)
    mesh->flagMask &= ~DG_COLOR;
  if (!haveTC)
    mesh->flagMask &= ~DG_TEXCOORD;
}
