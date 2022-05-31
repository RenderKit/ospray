// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/Intersect.ih"
#include "common/VolumeIntervals.ih"
#include "volume/Volume.ih"

export void Volume_embreeBounds(const RTCBoundsFunctionArguments *uniform args)
{
  Volume *uniform self = (Volume * uniform) args->geometryUserPtr;
  box3fa *uniform out = (box3fa * uniform) args->bounds_o;

  *out = make_box3fa(self->boundingBox);
}

export void Volume_intersect_kernel(
    const RTCIntersectFunctionNArguments *uniform args)
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

    EmbreeIntersectionContext *uniform ctx =
        (EmbreeIntersectionContext * uniform) args->context;
    varying VolumeIntervals *uniform vIntervals =
        (varying VolumeIntervals * uniform) ctx->userPtr;

    // Grow TLS pool if needed
    uniform unsigned int numNeeded =
        reduce_max(vIntervals->numVolumeIntervals) + 1;
    if (numNeeded > vIntervals->numAllocated) {
      vIntervals->intervals = (varying VolumeInterval * uniform) reallocTLS(
          vIntervals->intervals, numNeeded * sizeof(varying VolumeInterval));
      vIntervals->numAllocated = numNeeded;
    }

    varying VolumeInterval *varying vInterval = (varying VolumeInterval
        * varying)(vIntervals->intervals + vIntervals->numVolumeIntervals);

    vInterval->primID = 0;
    vInterval->geomID = args->geomID;
    vInterval->instID = args->context->instID[0];
    vInterval->interval.lower = t_min;
    vInterval->interval.upper = t_max;
    vIntervals->numVolumeIntervals++;
  }
}
