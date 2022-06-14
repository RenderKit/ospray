// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "common/Ray.ih"
#include "common/World.ih"
#include "geometry/Geometry.ih"
#include "openvkl/iterator.isph"
#include "rkcommon/math/box.ih"
#include "rkcommon/math/vec.ih"
#include "volume/VolumetricModel.ih"
// c++ shared
#include "IsosurfacesShared.h"

int Isosurfaces_primID(const Isosurfaces *uniform self, const float isovalue)
{
  for (uniform int i = 0; i < self->super.numPrimitives; ++i) {
    if (isovalue == self->isovalues[i]) {
      return i;
    }
  }
  return -1;
}

// A wrapper for backwards compatibility with the old VolumetricModel API
Volume *uniform Isosurfaces_getVolume(const Isosurfaces *uniform self)
{
  if (self->volume) {
    return self->volume;
  }
  return self->volumetricModel->volume;
}

export void Isosurfaces_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  uniform Isosurfaces *uniform self =
      (uniform Isosurfaces * uniform) args->geometryUserPtr;
  uniform int primID = args->primID;

  box3fa *uniform out = (box3fa * uniform) args->bounds_o;
  Volume *uniform volume = Isosurfaces_getVolume(self);
  *out = make_box3fa(volume->boundingBox);
}

void Isosurfaces_intersect_kernel(
    const RTCIntersectFunctionNArguments *uniform args,
    const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex])
    return;

  Isosurfaces *uniform self = (Isosurfaces * uniform) args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer toa varying ray!
  varying Ray *uniform ray = (varying Ray * uniform) args->rayhit;

  VKLHitIterator iterator;
  vkl_range1f tRange;
  tRange.lower = ray->t0;
  tRange.upper = ray->t;
  float time = 0.f;
  // do not use alloca: it prevents inlining and thus optimization
  // wrt. "template arg" isOcclusionTest
  uniform unsigned int8 hitIteratorBuffer[VKL_MAX_HIT_ITERATOR_SIZE];
  iterator = vklInitHitIteratorV(self->vklHitContext,
      (varying vkl_vec3f *)&ray->org,
      (varying vkl_vec3f *)&ray->dir,
      &tRange,
      &time,
      hitIteratorBuffer);
  VKLHit hit;
  bool gotHit = vklIterateHitV(iterator, &hit);

  if (gotHit) {
    if (isOcclusionTest) {
      ray->t = neg_inf;
    } else {
      ray->instID = args->context->instID[0];
      ray->geomID = args->geomID;
      ray->primID = Isosurfaces_primID(self, hit.sample);
      ray->t = hit.t;
      ray->u = hit.epsilon; // transport epsilon to postIntersect
      // transport hit point in *volume local coords* to postIntersect
      ray->Ng = ray->org + ray->dir * hit.t;
    }
  }
}

export void Isosurfaces_intersect(
    const struct RTCIntersectFunctionNArguments *uniform args)
{
  Isosurfaces_intersect_kernel(args, false);
}

export void Isosurfaces_occluded(
    const struct RTCOccludedFunctionNArguments *uniform args)
{
  Isosurfaces_intersect_kernel(
      (RTCIntersectFunctionNArguments * uniform) args, true);
}

void Isosurfaces_postIntersect(const Geometry *uniform geometry,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  Isosurfaces *uniform self = (Isosurfaces * uniform) geometry;
  Volume *uniform volume = Isosurfaces_getVolume(self);

  if (flags & (DG_NS | DG_NG))
    dg.Ng = dg.Ns = Volume_getGradient(volume, ray.Ng /* actually local hit */);

  // convert ray-space epsilon (in ray.u) to object-/world-space using max(dir)
  // instead of costly length; the error is at most sqrt(3)~1.7, quite
  // acceptable for eps
  dg.epsilon = reduce_max(abs(ray.dir)) * ray.u;

  if (and(flags & DG_COLOR, self->volumetricModel)) {
    TransferFunction *uniform xf = self->volumetricModel->transferFunction;
    dg.color = xf->get(xf, self->isovalues[ray.primID]);
  }
}

export void *uniform Isosurfaces_postIntersect_addr()
{
  return Isosurfaces_postIntersect;
}
