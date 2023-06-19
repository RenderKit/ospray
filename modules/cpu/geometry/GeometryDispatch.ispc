// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/DifferentialGeometry.ih"
#include "common/FeatureFlagsEnum.h"
#include "common/Ray.ih"
#include "common/RayQueryContext.ih"
#include "geometry/Boxes.ih"
#include "geometry/Curves.ih"
#include "geometry/GeometryDispatch.ih"
#ifdef OSPRAY_ENABLE_VOLUMES
#include "geometry/Isosurfaces.ih"
#endif
#include "geometry/Mesh.ih"
#include "geometry/Planes.ih"
#include "geometry/Spheres.ih"
#include "geometry/Subdivision.ih"
// c++ shared
#include "geometry/GeometryShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL void Geometry_dispatch_postIntersect(const Geometry *uniform self,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlags ff = getFeatureFlags(ffh);
  // See https://github.com/ispc/ispc/issues/2533
  const uniform FeatureFlagsGeometry ffg = ff.geometry;

  if ((self->type == GEOMETRY_TYPE_QUAD_MESH) && (ffg & FFG_QUAD)) {
    QuadMesh_postIntersect(self, dg, ray, flags);
  } else if ((self->type == GEOMETRY_TYPE_TRIANGLE_MESH)
      && (ffg & FFG_TRIANGLE)) {
    TriangleMesh_postIntersect(self, dg, ray, flags);
  } else if ((self->type == GEOMETRY_TYPE_BOXES) && (ffg & FFG_BOX)) {
    Boxes_postIntersect(self, dg, ray, flags);
  } else if ((self->type == GEOMETRY_TYPE_SPHERES) && (ffg & FFG_SPHERE)) {
    Spheres_postIntersect(self, dg, ray, flags);
  } else if ((self->type == GEOMETRY_TYPE_PLANES) && (ffg & FFG_PLANE)) {
    Planes_postIntersect(self, dg, ray, flags);
  } else if ((self->type == GEOMETRY_TYPE_CURVES) && (ffg & FFG_CURVES)) {
    Curves_postIntersect(self, dg, ray, flags);
#ifdef OSPRAY_ENABLE_VOLUMES
  } else if ((self->type == GEOMETRY_TYPE_ISOSURFACES)
      && (ffg & FFG_ISOSURFACE)) {
    Isosurfaces_postIntersect(self, dg, ray, flags, ffh);
#endif
#ifndef OSPRAY_TARGET_SYCL
  } else if ((self->type == GEOMETRY_TYPE_SUBDIVISION)
      && (ffg & FFG_SUBDIVISION)) {
    Subdivision_postIntersect(self, dg, ray, flags);
  } else {
    self->postIntersect(self, dg, ray, flags);
  }
#else
  } else {
  }
#endif
}

SYCL_EXTERNAL SampleAreaRes Geometry_dispatch_sampleArea(
    const Geometry *const uniform self,
    const int32 primID,
    const uniform affine3f &xfm,
    const uniform affine3f &rcp_xfm,
    const vec2f &s,
    const float time,
    const uniform FeatureFlagsHandler &ffh)

{
  const uniform FeatureFlags ff = getFeatureFlags(ffh);
  // See https://github.com/ispc/ispc/issues/2533
  const uniform FeatureFlagsGeometry ffg = ff.geometry;

  if (((self->type == GEOMETRY_TYPE_QUAD_MESH) && (ffg & FFG_QUAD))
      || ((self->type == GEOMETRY_TYPE_TRIANGLE_MESH)
          && (ffg & FFG_TRIANGLE))) {
    return Mesh_sampleArea(self, primID, xfm, rcp_xfm, s, time);
  } else if ((self->type == GEOMETRY_TYPE_SPHERES) && (ffg & FFG_SPHERE)) {
    return Spheres_sampleArea(self, primID, xfm, rcp_xfm, s, time);
  } else {
#ifndef OSPRAY_TARGET_SYCL
    return self->sampleArea(self, primID, xfm, rcp_xfm, s, time);
#endif
#ifdef OSPRAY_TARGET_SYCL
    SampleAreaRes res;
    res.pos = make_vec3f(0.f);
    res.normal = make_vec3f(0.f);
    return res;
#endif
  }
}

RTC_SYCL_INDIRECTLY_CALLABLE unmasked void Geometry_dispatch_intersect(
    RTCIntersectFunctionNArguments *uniform args)
{
  RayQueryContextDefault *uniform ctx =
      (RayQueryContextDefault * uniform) args->context;

  const uniform FeatureFlagsHandler &ffh = *ctx->ffh;
  const uniform FeatureFlags ff = getFeatureFlags(ffh);
  // See https://github.com/ispc/ispc/issues/2533
  const uniform FeatureFlagsGeometry ffg = ff.geometry;

  Geometry *uniform geom = (Geometry * uniform) args->geometryUserPtr;
  if ((geom->type == GEOMETRY_TYPE_BOXES) && (ffg & FFG_BOX)) {
    Boxes_intersect_kernel(args, false);
  } else if ((geom->type == GEOMETRY_TYPE_PLANES) && (ffg & FFG_PLANE)) {
    Planes_intersect_kernel(args, false);
#ifdef OSPRAY_ENABLE_VOLUMES
  } else if ((geom->type == GEOMETRY_TYPE_ISOSURFACES)
      && (ffg & FFG_ISOSURFACE)) {
    Isosurfaces_intersect_kernel(args, false, ffh);
#endif
  } else {
#ifndef OSPRAY_TARGET_SYCL
    geom->intersect((RTCIntersectFunctionNArguments * uniform) args, false);
#endif
  }
}

RTC_SYCL_INDIRECTLY_CALLABLE unmasked void Geometry_dispatch_occluded(
    RTCOccludedFunctionNArguments *uniform args)
{
  RayQueryContextDefault *uniform ctx =
      (RayQueryContextDefault * uniform) args->context;
  const uniform FeatureFlagsHandler &ffh = *ctx->ffh;
  const uniform FeatureFlags ff = getFeatureFlags(ffh);
  // See https://github.com/ispc/ispc/issues/2533
  const uniform FeatureFlagsGeometry ffg = ff.geometry;

  Geometry *uniform geom = (Geometry * uniform) args->geometryUserPtr;
  if ((geom->type == GEOMETRY_TYPE_BOXES) && (ffg & FFG_BOX)) {
    Boxes_intersect_kernel(
        (RTCIntersectFunctionNArguments * uniform) args, true);
  } else if ((geom->type == GEOMETRY_TYPE_PLANES) && (ffg & FFG_PLANE)) {
    Planes_intersect_kernel(
        (RTCIntersectFunctionNArguments * uniform) args, true);
#ifdef OSPRAY_ENABLE_VOLUMES
  } else if ((geom->type == GEOMETRY_TYPE_ISOSURFACES)
      && (ffg & FFG_ISOSURFACE)) {
    Isosurfaces_intersect_kernel(
        (RTCIntersectFunctionNArguments * uniform) args, true, ffh);
#endif
  } else {
#ifndef OSPRAY_TARGET_SYCL
    geom->intersect((RTCIntersectFunctionNArguments * uniform) args, true);
#endif
  }
}

OSPRAY_END_ISPC_NAMESPACE
