// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "common/Data.ih"
#include "common/DifferentialGeometry.ih"
#include "common/FilterIntersect.ih"
#include "common/Intersect.ih"
#include "common/Ray.ih"
#include "rkcommon/math/box.ih"
#include "rkcommon/math/vec.ih"
// c++ shared
#include "BoxesShared.h"

export void Boxes_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  Boxes *uniform self = (Boxes * uniform) args->geometryUserPtr;
  uniform int primID = args->primID;

  box3fa *uniform out = (box3fa * uniform) args->bounds_o;

  *out = make_box3fa(get_box3f(self->boxes, primID));
}

void Boxes_intersect_kernel(const RTCIntersectFunctionNArguments *uniform args,
    const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex])
    return;

  Boxes *uniform self = (Boxes * uniform) args->geometryUserPtr;
  varying Ray *uniform ray = (varying Ray * uniform) args->rayhit;

  uniform int primID = args->primID;

  uniform box3f box = get_box3f(self->boxes, primID);
  const Intersections isect = intersectBox(ray->org, ray->dir, box);

  // call intersection filtering callback and setup hit if accepted
  filterIntersectionBoth(args, isect, isOcclusionTest);
}

export void Boxes_intersect(
    const struct RTCIntersectFunctionNArguments *uniform args)
{
  Boxes_intersect_kernel(args, false);
}

export void Boxes_occluded(
    const struct RTCOccludedFunctionNArguments *uniform args)
{
  Boxes_intersect_kernel((RTCIntersectFunctionNArguments * uniform) args, true);
}

void Boxes_postIntersect(const Geometry *uniform geometry,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  dg.Ng = dg.Ns = ray.Ng;
}

export void *uniform Boxes_postIntersect_addr()
{
  return Boxes_postIntersect;
}