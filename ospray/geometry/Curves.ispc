// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "common/Data.ih"
#include "geometry/Geometry.ih"

struct Curves
{
  Geometry super; //!< inherited geometry fields
  Data1D index;
  Data1D vertex;
  Data1D color;
  Data1D texcoord;
  int64 flagMask;
  RTCGeometry geom;
};

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

void Curves_Constructor(Curves *uniform self, void *uniform cppEquivalent)
{
  Geometry_Constructor(&self->super, cppEquivalent, NULL);
  Data1D_Constructor(&self->index);
  Data1D_Constructor(&self->vertex);
  Data1D_Constructor(&self->color);
  Data1D_Constructor(&self->texcoord);
}

export void *uniform Curves_create(void *uniform cppEquivalent)
{
  Curves *uniform self = uniform new Curves;
  Curves_Constructor(self, cppEquivalent);
  return self;
}

export void *uniform Curves_set(void *uniform _self,
    RTCGeometry geom,
    uniform bool haveColor,
    uniform bool haveTexture,
    int32 uniform numSegments)
{
  Curves *uniform self = (Curves * uniform) _self;
  self->geom = geom;
  self->flagMask = -1;
  if (!haveColor)
    self->flagMask &= ~DG_COLOR;
  if (!haveTexture)
    self->flagMask &= ~DG_TEXCOORD;

  self->super.numPrimitives = numSegments;
  self->super.postIntersect = Curves_postIntersect;

  // TODO implement area sampling of OldCurves for geometry lights
  self->super.getAreas = NULL;
  self->super.sampleArea = NULL;
}