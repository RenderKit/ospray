// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "common/Data.ih"
#include "common/DifferentialGeometry.ih"
#include "common/FilterIntersect.ih"
#include "common/ISPCMessages.h"
#include "common/Intersect.ih"
#include "common/Ray.ih"
#include "geometry/Geometry.ih"
#include "math/sampling.ih"
#include "rkcommon/math/box.ih"
#include "rkcommon/math/vec.ih"
// c++ shared
#include "SpheresShared.h"

void Spheres_postIntersect(const Geometry *uniform geometry,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  Spheres *uniform self = (Spheres * uniform) geometry;

  dg.Ng = dg.Ns = ray.Ng;

  float radius = self->global_radius;
  if (valid(self->radius))
    radius = get_float(self->radius, ray.primID);

  // make epsilon large enough to not get lost when computing
  // |CO| = |center-ray.org| ~ radius for 2ndary rays
  dg.epsilon = radius * ulpEpsilon;

  if (and(flags & DG_TEXCOORD, valid(self->texcoord)))
    dg.st = get_vec2f(self->texcoord, ray.primID);
}

export void Spheres_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  Spheres *uniform self = (Spheres * uniform) args->geometryUserPtr;
  uniform int primID = args->primID;
  uniform float radius = valid(self->radius) ? get_float(self->radius, primID)
                                             : self->global_radius;
  uniform vec3f center = get_vec3f(self->vertex, primID);
  box3fa *uniform out = (box3fa * uniform) args->bounds_o;
  *out = make_box3fa(center - radius, center + radius);
}

void Spheres_intersect_kernel(
    const RTCIntersectFunctionNArguments *uniform args,
    const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex])
    return;

  Spheres *uniform self = (Spheres * uniform) args->geometryUserPtr;
  uniform int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer to a varying ray!
  varying Ray *uniform ray = (varying Ray * uniform) args->rayhit;

  uniform float radius = valid(self->radius) ? get_float(self->radius, primID)
                                             : self->global_radius;
  uniform vec3f center = get_vec3f(self->vertex, primID);

  const Intersections isect =
      intersectSphere(ray->org, ray->dir, center, radius);

  // call intersection filtering callback and setup hit if accepted
  filterIntersectionBoth(args, isect, isOcclusionTest);
}

export void Spheres_intersect(
    const struct RTCIntersectFunctionNArguments *uniform args)
{
  Spheres_intersect_kernel(args, false);
}

export void Spheres_occluded(
    const struct RTCOccludedFunctionNArguments *uniform args)
{
  Spheres_intersect_kernel((RTCIntersectFunctionNArguments *)args, true);
}

SampleAreaRes Spheres_sampleArea(const Geometry *uniform const _self,
    const int32 primID,
    const uniform affine3f &xfm,
    const uniform affine3f &rcp_xfm,
    const vec2f &s,
    const float)
{
  const Spheres *const uniform self = (const Spheres *uniform)_self;

  float radius = self->global_radius;
  if (valid(self->radius))
    radius = get_float(self->radius, primID);

  const vec3f localNormal = uniformSampleSphere(radius, s);
  const vec3f localPos = get_vec3f(self->vertex, primID) + localNormal;

  SampleAreaRes res;
  res.pos = xfmPoint(xfm, localPos);
  res.normal = normalize(xfmVector(transposed(rcp_xfm.l), localNormal));

  return res;
}

void Spheres_getAreas(const Geometry *const uniform _self,
    const int32 *const uniform primIDs,
    const uniform int32 numPrims,
    const uniform affine3f &xfm,
    float *const uniform areas)
{
  Spheres *uniform self = (Spheres * uniform) _self;

  // detect (non-uniform) scaling; get length of transformed unit-vectors
  const uniform vec3f scaling3 =
      make_vec3f(length(xfm.l.vx), length(xfm.l.vy), length(xfm.l.vz));
  const uniform float min_scaling = reduce_min(scaling3);
  const uniform float max_scaling = reduce_max(scaling3);
  if ((max_scaling - min_scaling) > 1e-4f * min_scaling)
    postStatusMsg(ISPC_MSG_SPHERES, OSP_LOG_WARNING);

  // use average as approximation to closest uniform scaling
  const uniform float scale = four_pi * sqr(reduce_add(scaling3) / 3.f);

  if (!valid(self->radius)) {
    const uniform float sphereArea = scale * sqr(self->global_radius);
    foreach (i = 0 ... numPrims)
      areas[i] = sphereArea;
  } else {
    // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
    for (uniform int32 i = 0; i < numPrims; i++)
      areas[i] = scale * sqr(get_float(self->radius, i));
  }
}

export void *uniform Spheres_postIntersect_addr()
{
  return Spheres_postIntersect;
}

export void *uniform Spheres_sampleArea_addr()
{
  return Spheres_sampleArea;
}

export void *uniform Spheres_getAreas_addr()
{
  return Spheres_getAreas;
}
