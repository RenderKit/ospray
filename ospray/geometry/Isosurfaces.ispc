// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "common/Ray.ih"
#include "common/World.ih"
#include "geometry/Geometry.ih"
#include "math/box.ih"
#include "math/vec.ih"
#include "openvkl/iterator.isph"
#include "volume/Volume.ih"

struct Isosurfaces
{
  Geometry super; //!< inherited geometry fields
  float *isovalues;
  // For backwards compatability, a volumetric model was used to set
  // the volume and color
  VolumetricModel *volumetricModel;
  Volume *volume;
  VKLValueSelector valueSelector;
};

int Isosurfaces_primID(const Isosurfaces *uniform self, const float isovalue)
{
  for (uniform int i = 0; i < self->super.numPrimitives; ++i) {
    if (isovalue == self->isovalues[i]) {
      return i;
    }
  }
  return -1;
}

// A wrapper for backwards compatability with the old VolumetricModel API
Volume *uniform Isosurfaces_getVolume(const Isosurfaces *uniform self)
{
  if (self->volume) {
    return self->volume;
  }
  return self->volumetricModel->volume;
}

unmasked void Isosurfaces_bounds(const RTCBoundsFunctionArguments *uniform args)
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

  Volume *uniform volume = Isosurfaces_getVolume(self);

  VKLHitIterator iterator;
  vkl_range1f tRange;
  tRange.lower = ray->t0;
  tRange.upper = ray->t;
  uniform unsigned int8 hitIteratorBuffer[VKL_MAX_HIT_ITERATOR_SIZE];
  iterator = vklInitHitIteratorV(volume->vklSampler,
      (varying vkl_vec3f *)&ray->org,
      (varying vkl_vec3f *)&ray->dir,
      &tRange,
      self->valueSelector,
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

unmasked void Isosurfaces_intersect(
    const struct RTCIntersectFunctionNArguments *uniform args)
{
  Isosurfaces_intersect_kernel(args, false);
}

unmasked void Isosurfaces_occluded(
    const struct RTCOccludedFunctionNArguments *uniform args)
{
  Isosurfaces_intersect_kernel(
      (RTCIntersectFunctionNArguments * uniform) args, true);
}

static void Isosurfaces_postIntersect(const Geometry *uniform geometry,
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

export void *uniform Isosurfaces_create(void *uniform cppEquivalent)
{
  uniform Isosurfaces *uniform isosurfaces = uniform new uniform Isosurfaces;
  Geometry_Constructor(
      &isosurfaces->super, cppEquivalent, Isosurfaces_postIntersect);
  return isosurfaces;
}

export void *uniform Isosurfaces_set(void *uniform _self,
    void *uniform _embreeGeometry,
    int32 uniform numIsovalues,
    uniform float *uniform isovalues,
    void *uniform _volumetricModel,
    void *uniform _volume,
    void *uniform _valueSelector)
{
  Isosurfaces *uniform self = (Isosurfaces * uniform) _self;
  VolumetricModel *uniform volumetricModel =
      (VolumetricModel * uniform) _volumetricModel;
  Volume *uniform volume = (Volume * uniform) _volume;
  VKLValueSelector valueSelector = (VKLValueSelector)_valueSelector;

  self->isovalues = isovalues;
  self->volumetricModel = volumetricModel;
  self->volume = volume;
  self->super.numPrimitives = 1; // for embree it is just one prim
  self->valueSelector = valueSelector;

  Geometry_setEmbreeUserGeometry(&self->super,
      (RTCGeometry)_embreeGeometry,
      Isosurfaces_bounds,
      Isosurfaces_intersect,
      Isosurfaces_occluded);

  self->super.numPrimitives = numIsovalues;
}
