// Copyright 2019-2020 Intel Corporation
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

// The plane can't be truely infinite because Embree has a limit on object size
#define PLANE_SIZE 1e18

struct Planes
{
  Geometry super;
  Data1D planes;
};

unmasked void Planes_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  box3fa *uniform out = (box3fa * uniform) args->bounds_o;
  const uniform float lo = -PLANE_SIZE;
  const uniform float hi = PLANE_SIZE;
  *out = make_box3fa(make_vec3f(lo, lo, lo), make_vec3f(hi, hi, hi));
}

void Planes_intersect_kernel(const RTCIntersectFunctionNArguments *uniform args,
    const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex])
    return;

  // get geometry pointers
  Planes *uniform self = (Planes * uniform) args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer to a varying ray!
  varying Ray *uniform ray = (varying Ray * uniform) args->rayhit;

  // intersect plane with ray
  uniform vec4f coeffs = get_vec4f(self->planes, primID);
  const Hit hit = intersectPlane(ray->org, ray->dir, coeffs);

  // we are done if first intersection accepted
  if (filterIntersectionSingle(args, hit, isOcclusionTest, false))
    return;

  // clipping algorithm works on closed surfaces only,
  // so for planes an implicit second plane is tested here,
  // it is positioned at the edge of bounding box and its
  // normal is oriented in opposite direction to mimic a closed surface
  const uniform vec4f coeffsImplicit =
      make_vec4f(-coeffs.x, -coeffs.y, -coeffs.z, PLANE_SIZE);
  const Hit hitImplicit = intersectPlane(ray->org, ray->dir, coeffsImplicit);

  // we don't want the second plane to be drawn so this intersection is never
  // accepted
  filterIntersectionSingle(args, hitImplicit, isOcclusionTest, true);
}

unmasked void Planes_intersect(
    const struct RTCIntersectFunctionNArguments *uniform args)
{
  Planes_intersect_kernel(args, false);
}

unmasked void Planes_occluded(
    const struct RTCOccludedFunctionNArguments *uniform args)
{
  Planes_intersect_kernel(
      (RTCIntersectFunctionNArguments * uniform) args, true);
}

static void Planes_postIntersect(const Geometry *uniform geometry,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  dg.Ng = dg.Ns = ray.Ng;
}

export void *uniform Planes_create(void *uniform cppEquivalent)
{
  Planes *uniform self = uniform new Planes;

  Geometry_Constructor(&self->super, cppEquivalent, Planes_postIntersect);
  Data1D_Constructor(&self->planes);

  return self;
}

export void *uniform Planes_set(void *uniform _self,
    void *uniform _embreeGeometry,
    const Data1D *uniform planes)
{
  Planes *uniform self = (Planes * uniform) _self;

  self->planes = *planes;
  self->super.numPrimitives = planes->numItems;

  Geometry_setEmbreeUserGeometry(&self->super,
      (RTCGeometry)_embreeGeometry,
      Planes_bounds,
      Planes_intersect,
      Planes_occluded);
}
