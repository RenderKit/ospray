// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "common/Data.ih"
#include "common/DifferentialGeometry.ih"
#include "common/FilterIntersect.ih"
#include "common/Intersect.ih"
#include "common/Ray.ih"
#include "geometry/Geometry.ih"
#include "rkcommon/math/box.ih"
#include "rkcommon/math/vec.ih"
// c++ shared
#include "PlanesShared.h"

// The plane can't be truly infinite because Embree has a limit on object size
#define PLANE_MAX_SIZE 1e18
#define DUMMY_MAX_T 1e38

export void Planes_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  const uniform vec3f lo =
      make_vec3f(-PLANE_MAX_SIZE, -PLANE_MAX_SIZE, -PLANE_MAX_SIZE);
  const uniform vec3f hi =
      make_vec3f(PLANE_MAX_SIZE, PLANE_MAX_SIZE, PLANE_MAX_SIZE);

  // use maximum size planes if bounding boxes not provided
  Planes *uniform self = (Planes * uniform) args->geometryUserPtr;
  box3fa *uniform out = (box3fa * uniform) args->bounds_o;
  if (valid(self->bounds)) {
    uniform int primID = args->primID;
    uniform box3f bbox = get_box3f(self->bounds, primID);
    *out = make_box3fa(max(bbox.lower, lo), min(bbox.upper, hi));
  } else {
    *out = make_box3fa(lo, hi);
  }
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

  // this assumes that the args->rayhit is actually a pointer to a varying
  // ray!
  varying Ray *uniform ray = (varying Ray * uniform) args->rayhit;

  // intersect plane with ray
  uniform vec4f coeffs = get_vec4f(self->coeffs, primID);
  const Hit hit = intersectPlane(ray->org, ray->dir, coeffs);

  // do bounding box check if defined
  if (valid(self->bounds)) {
    uniform box3f bbox = get_box3f(self->bounds, primID);
    const Intersections isect = intersectBox(ray->org, ray->dir, bbox);

    // discard the intersection if our ray misses bbox completely or
    // the intersection is not within bbox extents,
    // assumes that (isect.entry.hit == isect.exit.hit)
    if (!isect.entry.hit || (hit.t < isect.entry.t) || (hit.t > isect.exit.t))
      return;
  }

  // we are done if first intersection accepted
  if (filterIntersectionSingle(args, hit, isOcclusionTest, false))
    return;

  // handle plane parallel to ray, which breaks interval logic for clipping
  if (!hit.hit) {
    const uniform vec3f normal = make_vec3f(coeffs);
    if (coeffs.w > dot(ray->org, normal)) { // within clipping half-space?
      Hit hitExitDummy;
      hitExitDummy.hit = true;
      hitExitDummy.t = DUMMY_MAX_T;
      hitExitDummy.N = ray->dir;
      filterIntersectionSingle(args, hitExitDummy, isOcclusionTest, true);
    }
    return;
  }

  // The second implicit plane is needed just for infinite planes
  if (valid(self->bounds))
    return;

  // clipping algorithm works on closed surfaces only,
  // so for infinite planes an implicit second plane is tested here,
  // it is positioned at the edge of bounding box and its
  // normal is oriented in opposite direction to mimic a closed surface
  const uniform vec4f coeffsImplicit =
      make_vec4f(-coeffs.x, -coeffs.y, -coeffs.z, PLANE_MAX_SIZE);
  const Hit hitImplicit = intersectPlane(ray->org, ray->dir, coeffsImplicit);

  // we don't want the second plane to be drawn so this intersection is never
  // accepted
  filterIntersectionSingle(args, hitImplicit, isOcclusionTest, true);
}

export void Planes_intersect(
    const struct RTCIntersectFunctionNArguments *uniform args)
{
  Planes_intersect_kernel(args, false);
}

export void Planes_occluded(
    const struct RTCOccludedFunctionNArguments *uniform args)
{
  Planes_intersect_kernel(
      (RTCIntersectFunctionNArguments * uniform) args, true);
}

void Planes_postIntersect(const Geometry *uniform geometry,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  dg.Ng = dg.Ns = ray.Ng;
}

export void *uniform Planes_postIntersect_addr()
{
  return Planes_postIntersect;
}
