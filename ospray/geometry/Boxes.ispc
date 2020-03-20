// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "common/Data.ih"
#include "common/FilterIntersect.ih"
#include "common/Intersect.ih"
#include "common/Ray.ih"
#include "common/World.ih"
#include "geometry/Geometry.ih"
#include "math/box.ih"
#include "math/vec.ih"

struct Boxes
{
  Geometry super;
  Data1D boxes;
};

unmasked void Boxes_bounds(const RTCBoundsFunctionArguments *uniform args)
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

unmasked void Boxes_intersect(
    const struct RTCIntersectFunctionNArguments *uniform args)
{
  Boxes_intersect_kernel(args, false);
}

unmasked void Boxes_occluded(
    const struct RTCOccludedFunctionNArguments *uniform args)
{
  Boxes_intersect_kernel((RTCIntersectFunctionNArguments * uniform) args, true);
}

static void Boxes_postIntersect(const Geometry *uniform geometry,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  dg.Ng = dg.Ns = ray.Ng;
}

export void *uniform Boxes_create(void *uniform cppEquivalent)
{
  Boxes *uniform self = uniform new Boxes;

  Geometry_Constructor(&self->super, cppEquivalent, Boxes_postIntersect);
  Data1D_Constructor(&self->boxes);

  return self;
}

export void *uniform Boxes_set(void *uniform _self,
    void *uniform _embreeGeometry,
    const Data1D *uniform boxes)
{
  Boxes *uniform self = (Boxes * uniform) _self;

  self->boxes = *boxes;
  self->super.numPrimitives = boxes->numItems;

  Geometry_setEmbreeUserGeometry(&self->super,
      (RTCGeometry)_embreeGeometry,
      Boxes_bounds,
      Boxes_intersect,
      Boxes_occluded);
}
