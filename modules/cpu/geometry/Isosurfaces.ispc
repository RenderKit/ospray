// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#include "common/FeatureFlagsEnum.h"
#ifdef OSPRAY_ENABLE_VOLUMES

// ospray
#include "common/FilterIntersect.ih"
#include "common/Ray.ih"
#include "common/World.ih"
#include "geometry/Geometry.ih"
#include "rkcommon/math/box.ih"
#include "rkcommon/math/vec.ih"
#include "volume/Volume.ih"
#include "volume/VolumetricModel.ih"
#include "volume/transferFunction/TransferFunctionDispatch.ih"
// c++ shared
#include "IsosurfacesShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

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

  box3fa *uniform out = (box3fa * uniform) args->bounds_o;
  Volume *uniform volume = Isosurfaces_getVolume(self);
  *out = make_box3fa(volume->boundingBox);
}

SYCL_EXTERNAL void Isosurfaces_postIntersect(const Geometry *uniform geometry,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags,
    const uniform FeatureFlagsHandler &ffh)
{
  Isosurfaces *uniform self = (Isosurfaces * uniform) geometry;
  Volume *uniform volume = Isosurfaces_getVolume(self);

  if (flags & (DG_NS | DG_NG))
    dg.Ng = dg.Ns =
        Volume_getGradient(volume, ray.Ng /* actually local hit */, ffh);

  // convert ray-space epsilon (in ray.u) to object-/world-space using max(dir)
  // instead of costly length; the error is at most sqrt(3)~1.7, quite
  // acceptable for eps
  dg.epsilon = reduce_max(abs(ray.dir)) * ray.u;

  if ((flags & DG_COLOR) && self->volumetricModel) {
    TransferFunction *uniform xf = self->volumetricModel->transferFunction;
    dg.color = TransferFunction_dispatch_get(xf, self->isovalues[ray.primID]);
  }
}

SYCL_EXTERNAL void unmasked Isosurfaces_intersect_kernel(
    const RTCIntersectFunctionNArguments *uniform args,
    const uniform bool isOcclusionTest,
    const uniform FeatureFlagsHandler &ffh)
{
  // make sure to set the mask
  if (!args->valid[programIndex]) {
    return;
  }

  const uniform FeatureFlags ff = getFeatureFlags(ffh);

  args->valid[programIndex] = 0;

  Isosurfaces *uniform self = (Isosurfaces * uniform) args->geometryUserPtr;

  // this assumes that the args->rayhit is actually a pointer to a varying ray!
  varying Ray *uniform ray = (varying Ray * uniform) args->rayhit;

  VKLHitIterator iterator;
  vkl_range1f tRange;
  tRange.lower = ray->t0;
  tRange.upper = ray->t;
  float time = 0.f;
  // do not use alloca: it prevents inlining and thus optimization
  // wrt. "template arg" isOcclusionTest, alloca also not supported on GPU
  uniform uint8 hitIteratorBuffer[VKL_MAX_HIT_ITERATOR_SIZE];

#if OPENVKL_VERSION_MAJOR == 1
  // We know if we have OpenVKL version 1 we're not targetting SYCL
  iterator = vklInitHitIteratorV(self->vklHitContext,
      (varying vkl_vec3f *)&ray->org,
      (varying vkl_vec3f *)&ray->dir,
      &tRange,
      &time,
      hitIteratorBuffer);
#else
  iterator = vklInitHitIteratorV(&self->vklHitContext,
      (varying vkl_vec3f *)&ray->org,
      (varying vkl_vec3f *)&ray->dir,
      &tRange,
#ifdef OSPRAY_TARGET_SYCL
      time,
#else
      &time,
#endif
      hitIteratorBuffer
#ifdef OSPRAY_TARGET_SYCL
      ,
      ff.volume
#endif
  );
#endif

  VKLHit hit;
  bool gotHit = vklIterateHitV(iterator,
      &hit
#ifdef OSPRAY_TARGET_SYCL
      ,
      ff.volume
#endif
  );
  if (!gotHit)
    return;

  Hit hitFilter;
  hitFilter.hit = true;
  hitFilter.t = hit.t;
  // Transport hit point in *volume local coords* to postIntersect
  hitFilter.N = ray->org + ray->dir * hit.t;
  // Transport epsilon to postIntersect
  hitFilter.u = hit.epsilon;
  bool accept =
      filterIntersectionSingle(args, hitFilter, isOcclusionTest, false);
  if (!accept)
    return;

  args->valid[programIndex] = -1;

  if (isOcclusionTest) {
    ray->t = neg_inf;
  } else {
    ray->instID = args->context->instID[0];
    ray->geomID = args->geomID;
    ray->primID = Isosurfaces_primID(self, hit.sample);
    ray->t = hitFilter.t;
    ray->u = hitFilter.u;
    ray->Ng = hitFilter.N;
  }
}

export void *uniform Isosurfaces_postIntersect_addr()
{
  return (void *uniform)Isosurfaces_postIntersect;
}

OSPRAY_END_ISPC_NAMESPACE
#endif
