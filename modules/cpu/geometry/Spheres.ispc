// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Spheres.ih"
#include "common/Data.ih"
#include "common/DifferentialGeometry.ih"
#include "common/FilterIntersect.ih"
#include "common/ISPCMessages.h"
#include "common/Intersect.ih"
#include "common/Ray.ih"
#include "math/sampling.ih"
#include "rkcommon/math/box.ih"
#include "rkcommon/math/vec.ih"
// c++ shared
#include "SpheresShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

#ifdef OSPRAY_TARGET_SYCL
using namespace ospray;
#endif

SYCL_EXTERNAL void Spheres_postIntersect(const Geometry *uniform geometry,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  Spheres *uniform self = (Spheres * uniform) geometry;

  dg.Ng = dg.Ns = ray.Ng;

  const vec4f vertex = get_vec4f(self->sphere, ray.primID);

  // make epsilon large enough to not get lost when computing
  // |CO| = |center-ray.org| ~ radius for 2ndary rays
  dg.epsilon = vertex.w * ulpEpsilon;

  if ((flags & DG_TEXCOORD) && valid(self->texcoord)) {
    dg.st = get_vec2f(self->texcoord, ray.primID);
  }
}

SYCL_EXTERNAL SampleAreaRes Spheres_sampleArea(
    const Geometry *uniform const _self,
    const int32 primID,
    const uniform affine3f &xfm,
    const uniform affine3f &rcp_xfm,
    const vec2f &s,
    const float)
{
  const Spheres *const uniform self = (const Spheres *uniform)_self;

  const vec4f vertex = get_vec4f(self->sphere, primID);
  vec3f localNormal;
  vec3f localPos;
  if (self->sphereType == OSP_ORIENTED_DISC) {
    localNormal = normalize(get_vec3f(self->normalData, primID));
    localPos = frame(localNormal) * uniformSampleDisk(vertex.w, s);
  } else {
    localNormal = uniformSampleSphere(vertex.w, s);
    localPos = localNormal;
  }

  SampleAreaRes res;
  res.pos = xfmPoint(xfm, make_vec3f(vertex) + localPos);
  res.normal = normalize(xfmVector(transposed(rcp_xfm.l), localNormal));
  res.st = valid(self->texcoord) ? get_vec2f(self->texcoord, primID)
                                 : make_vec2f(0.f);

  return res;
}

void Spheres_getAreas(const Geometry *const uniform _self,
    const int32 *const uniform,
    const uniform int32 numPrims,
    const uniform affine3f &xfm,
    float *const uniform areas)
{
  Spheres *uniform self = (Spheres * uniform) _self;

  // detect (non-uniform) scaling; get length of transformed unit-vectors
  const uniform vec3f scaling3 =
      make_vec3f(length(xfm.l.vx), length(xfm.l.vy), length(xfm.l.vz));
#if 0
  const uniform float min_scaling = reduce_min(scaling3);
  const uniform float max_scaling = reduce_max(scaling3);
  if ((max_scaling - min_scaling) > 1e-4f * min_scaling) {
    postStatusMsg(ISPC_MSG_SPHERES, OSP_LOG_WARNING);
  }
#endif

  // use average as approximation to closest uniform scaling
  const uniform float scale = ((self->sphereType == OSP_SPHERE) ? 4.0f : 1.0f)
      * (float)pi * sqr(reduce_add(scaling3) / 3.f);

  // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
  for (uniform int32 i = 0; i < numPrims; i++) {
    const uniform vec4f vertex = get_vec4f(self->sphere, i);
    areas[i] = scale * sqr(vertex.w);
  }
}

export void *uniform Spheres_postIntersect_addr()
{
  return (void *uniform)Spheres_postIntersect;
}

export void *uniform Spheres_sampleArea_addr()
{
  return (void *uniform)Spheres_sampleArea;
}

export void *uniform Spheres_getAreas_addr()
{
  return (void *uniform)Spheres_getAreas;
}

OSPRAY_END_ISPC_NAMESPACE
