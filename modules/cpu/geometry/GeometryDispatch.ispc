// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/DifferentialGeometry.ih"
#include "common/Ray.ih"
#include "geometry/Boxes.ih"
#include "geometry/Curves.ih"
#include "geometry/GeometryDispatch.ih"
#include "geometry/Mesh.ih"
#include "geometry/Planes.ih"
#include "geometry/Spheres.ih"
#include "geometry/Subdivision.ih"
// TODO: No iterators in SYCL API yet
#if defined(OSPRAY_ENABLE_VOLUMES) && !defined(OSPRAY_TARGET_SYCL)
#include "geometry/Isosurfaces.ih"
#endif
// c++ shared
#include "geometry/GeometryShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL __noinline void Geometry_dispatch_postIntersect(
    const Geometry *uniform self,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  switch (self->type) {
  case GEOMETRY_TYPE_QUAD_MESH:
    QuadMesh_postIntersect(self, dg, ray, flags);
    break;
  case GEOMETRY_TYPE_TRIANGLE_MESH:
    TriangleMesh_postIntersect(self, dg, ray, flags);
    break;
  case GEOMETRY_TYPE_BOXES:
    Boxes_postIntersect(self, dg, ray, flags);
    break;
  case GEOMETRY_TYPE_SPHERES:
    Spheres_postIntersect(self, dg, ray, flags);
    break;
  case GEOMETRY_TYPE_PLANES:
    Planes_postIntersect(self, dg, ray, flags);
    break;
  case GEOMETRY_TYPE_CURVES:
    Curves_postIntersect(self, dg, ray, flags);
    break;
// TODO: No iterators in SYCL API yet
#if defined(OSPRAY_ENABLE_VOLUMES) && !defined(OSPRAY_TARGET_SYCL)
  case GEOMETRY_TYPE_ISOSURFACES:
    Isosurfaces_postIntersect(self, dg, ray, flags);
    break;
#endif
#ifndef OSPRAY_TARGET_SYCL
    // Subdivision surfaces not supported on the GPU right now
  case GEOMETRY_TYPE_SUBDIVISION:
    Subdivision_postIntersect(self, dg, ray, flags);
    break;
#endif
  default:
#ifndef OSPRAY_TARGET_SYCL
    self->postIntersect(self, dg, ray, flags);
#endif
    break;
  }
}

SYCL_EXTERNAL __noinline SampleAreaRes Geometry_dispatch_sampleArea(
    const Geometry *const uniform self,
    const int32 primID,
    const uniform affine3f &xfm,
    const uniform affine3f &rcp_xfm,
    const vec2f &s,
    const float time)

{
  switch (self->type) {
  case GEOMETRY_TYPE_QUAD_MESH:
  case GEOMETRY_TYPE_TRIANGLE_MESH:
    return Mesh_sampleArea(self, primID, xfm, rcp_xfm, s, time);
  case GEOMETRY_TYPE_SPHERES:
    return Spheres_sampleArea(self, primID, xfm, rcp_xfm, s, time);
  default:
#ifndef OSPRAY_TARGET_SYCL
    return self->sampleArea(self, primID, xfm, rcp_xfm, s, time);
#endif
    break;
  }
#ifdef OSPRAY_TARGET_SYCL
  SampleAreaRes res;
  res.pos = make_vec3f(0.f);
  res.normal = make_vec3f(0.f);
  return res;
#endif
}

RTC_SYCL_INDIRECTLY_CALLABLE unmasked void Geometry_dispatch_intersect(
    RTCIntersectFunctionNArguments *uniform args)
{
  Geometry *uniform geom = (Geometry * uniform) args->geometryUserPtr;
  switch (geom->type) {
  case GEOMETRY_TYPE_BOXES:
    Boxes_intersect_kernel(args, false);
    break;
  case GEOMETRY_TYPE_PLANES:
    Planes_intersect_kernel(args, false);
    break;
#if 0
  case GEOMETRY_TYPE_SPHERES:
    Spheres_intersect_kernel(args, false);
    break;
#endif
// TODO: No iterators in SYCL API yet
#if defined(OSPRAY_ENABLE_VOLUMES) && !defined(OSPRAY_TARGET_SYCL)
  case GEOMETRY_TYPE_ISOSURFACES:
    Isosurfaces_intersect_kernel(args, false);
    break;
#endif
  default:
#ifndef OSPRAY_TARGET_SYCL
    geom->intersect((RTCIntersectFunctionNArguments * uniform) args, false);
#endif
    break;
  }
}

RTC_SYCL_INDIRECTLY_CALLABLE unmasked void Geometry_dispatch_occluded(
    RTCOccludedFunctionNArguments *uniform args)
{
  Geometry *uniform geom = (Geometry * uniform) args->geometryUserPtr;
  switch (geom->type) {
  case GEOMETRY_TYPE_BOXES:
    Boxes_intersect_kernel(
        (RTCIntersectFunctionNArguments * uniform) args, true);
    break;
  case GEOMETRY_TYPE_PLANES:
    Planes_intersect_kernel(
        (RTCIntersectFunctionNArguments * uniform) args, true);
    break;
#if 0
  case GEOMETRY_TYPE_SPHERES:
    Spheres_intersect_kernel(
        (RTCIntersectFunctionNArguments * uniform) args, true);
    break;
#endif
// TODO: No iterators in SYCL API yet
#if defined(OSPRAY_ENABLE_VOLUMES) && !defined(OSPRAY_TARGET_SYCL)
  case GEOMETRY_TYPE_ISOSURFACES:
    Isosurfaces_intersect_kernel(
        (RTCIntersectFunctionNArguments * uniform) args, true);
    break;
#endif
  default:
#ifndef OSPRAY_TARGET_SYCL
    geom->intersect((RTCIntersectFunctionNArguments * uniform) args, true);
#endif
    break;
  }
}

OSPRAY_END_ISPC_NAMESPACE
