// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#ifdef OSPRAY_ENABLE_VOLUMES

#include "common/Intersect.ih"
#include "common/Ray.ih"
#include "common/VolumeIntervals.ih"
#include "volume/Volume.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

export void Volume_embreeBounds(const void *uniform _args)
{
  const RTCBoundsFunctionArguments *uniform args =
      (const RTCBoundsFunctionArguments *uniform)_args;
  Volume *uniform self = (Volume * uniform) args->geometryUserPtr;
  box3fa *uniform out = (box3fa * uniform) args->bounds_o;

  *out = make_box3fa(self->boundingBox);
}

RTC_SYCL_INDIRECTLY_CALLABLE unmasked void Volume_intersect_kernel(
    RTCIntersectFunctionNArguments *uniform args)
{
  // make sure to set the mask
  if (!args->valid[programIndex])
    return;

  Volume *uniform self = (Volume * uniform) args->geometryUserPtr;
  varying Ray *uniform ray = (varying Ray * uniform) args->rayhit;

  box3f *uniform box = &self->boundingBox;

  const Intersections isect = intersectBox(ray->org, ray->dir, *box);

  const float t_min = max(ray->t0, isect.entry.t);
  const float t_max = min(ray->t, isect.exit.t);

  if (t_min < t_max) {
    ray->instID = args->context->instID[0];
    ray->geomID = args->geomID;
    ray->primID = 0;

    RayQueryContextVolume *uniform ctx =
        (RayQueryContextVolume * uniform) args->context;
    varying VolumeIntervals *uniform vIntervals =
        (varying VolumeIntervals * uniform) ctx->intervals;

#ifndef OSPRAY_TARGET_SYCL
    // Grow TLS pool if needed, note that we cannot grow it on the GPU b/c no
    // dymanic memory. With SYCL we just take the first hit found
    uniform unsigned int numNeeded =
        reduce_max(vIntervals->numVolumeIntervals) + 1;
    if (numNeeded > vIntervals->numAllocated) {
      vIntervals->intervals = (varying VolumeInterval * uniform) reallocTLS(
          vIntervals->intervals, numNeeded * sizeof(varying VolumeInterval));
      vIntervals->numAllocated = numNeeded;
    }

    varying VolumeInterval *varying vInterval = (varying VolumeInterval
        * varying)(vIntervals->intervals + vIntervals->numVolumeIntervals);
#else
    varying VolumeInterval *uniform vInterval = vIntervals->intervals;
#endif

    vInterval->primID = 0;
    vInterval->geomID = args->geomID;
    vInterval->instID = args->context->instID[0];
    vInterval->interval.lower = t_min;
    vInterval->interval.upper = t_max;
    vIntervals->numVolumeIntervals++;
  }
}

OSPRAY_END_ISPC_NAMESPACE
#endif
