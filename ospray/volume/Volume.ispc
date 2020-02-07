// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/Intersect.ih"
#include "volume/Volume.ih"

unmasked void Volume_embreeBounds(
    const RTCBoundsFunctionArguments *uniform args)
{
  Volume *uniform self = (Volume * uniform) args->geometryUserPtr;
  box3fa *uniform out = (box3fa * uniform) args->bounds_o;

  *out = make_box3fa(self->boundingBox);
}

unmasked void Volume_intersect_kernel(
    const RTCIntersectFunctionNArguments *uniform args)
{
  // make sure to set the mask
  if (!args->valid[programIndex])
    return;

  Volume *uniform self = (Volume * uniform) args->geometryUserPtr;
  varying Ray *uniform ray = (varying Ray * uniform) args->rayhit;

  EmbreeIntersectionContext *uniform ctx =
      (EmbreeIntersectionContext * uniform) args->context;

  varying VolumeIntervals *uniform vIntervals =
      (varying VolumeIntervals * uniform) ctx->userPtr;

  varying VolumeInterval *varying vInterval = (varying VolumeInterval
      * varying)(vIntervals->intervals + vIntervals->numVolumeIntervals);

  uniform int primID = args->primID;

  box3f *uniform box = &self->boundingBox;

  const Intersections isect = intersectBox(ray->org, ray->dir, *box);

  const float t_min = max(ray->t0, isect.entry.t);
  const float t_max = min(ray->t, isect.exit.t);

  if (t_min < t_max) {
    ray->instID = args->context->instID[0];
    ray->geomID = self->volumeID;
    ray->primID = 0;

    vInterval->primID = 0;
    vInterval->geomID = self->volumeID;
    vInterval->instID = args->context->instID[0];
    vIntervals->numVolumeIntervals++;
    vInterval->interval.lower = t_min;
    vInterval->interval.upper = t_max;
  }
}

void Volume_Constructor(Volume *uniform self, void *uniform cppEquivalent)
{
  self->cppEquivalent = cppEquivalent;
  self->boundingBox = make_box3f(make_vec3f(0.f), make_vec3f(1.f));
  self->volumeID = -1;
  self->vklVolume = NULL;
}

export void Volume_getBoundingBox(
    uniform box3f *uniform ret, void *uniform _self)
{
  uniform Volume *uniform self = (uniform Volume * uniform) _self;

  *ret = self->boundingBox;
}

export void Volume_set(void *uniform _self, void *uniform _embreeGeometry)
{
  Volume *uniform self = (Volume * uniform) _self;

  RTCGeometry geom = (RTCGeometry)_embreeGeometry;

  rtcSetGeometryUserData(geom, self);
  rtcSetGeometryUserPrimitiveCount(geom, 1);
  rtcSetGeometryBoundsFunction(
      geom, (uniform RTCBoundsFunction)&Volume_embreeBounds, self);
  rtcSetGeometryIntersectFunction(
      geom, (uniform RTCIntersectFunctionN)&Volume_intersect_kernel);
  rtcCommitGeometry(geom);
}

export void Volume_set_geomID(void *uniform _self, uniform int32 geomID)
{
  Volume *uniform self = (Volume * uniform) _self;
  self->volumeID = geomID;
}

export void *uniform Volume_createInstance_vklVolume(
    void *uniform cppEquivalent)
{
  Volume *uniform self = uniform new Volume;
  Volume_Constructor(self, cppEquivalent);
  return self;
}

export void Volume_set_vklVolume(
    void *uniform _self, void *uniform vklVolume, box3f *uniform bounds)
{
  Volume *uniform self = (Volume * uniform) _self;
  self->vklVolume = (VKLVolume)vklVolume;
  self->boundingBox = *bounds;
}
