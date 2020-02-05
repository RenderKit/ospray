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
  VolumetricModel *volumetricModel;
  VKLValueSelector valueSelector;
};

unmasked void Isosurfaces_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  uniform Isosurfaces *uniform self =
      (uniform Isosurfaces * uniform) args->geometryUserPtr;
  uniform int primID = args->primID;

  box3fa *uniform out = (box3fa * uniform) args->bounds_o;
  *out = make_box3fa(self->volumetricModel->volume->boundingBox);
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

  Volume *uniform volume = self->volumetricModel->volume;

  VKLHitIterator iterator;
  vkl_range1f tRange;
  tRange.lower = ray->t0;
  tRange.upper = ray->t;
  vklInitHitIteratorV(&iterator,
      self->volumetricModel->volume->vklVolume,
      (varying vkl_vec3f *)&ray->org,
      (varying vkl_vec3f *)&ray->dir,
      &tRange,
      self->valueSelector);
  VKLHit hit;
  bool gotHit = vklIterateHitV(&iterator, &hit);

  if (gotHit) {
    if (isOcclusionTest) {
      ray->t = neg_inf;
    } else {
      ray->instID = args->context->instID[0];
      ray->geomID = args->geomID;
      ray->primID = 0; // ??? which primID?
      ray->t = hit.t;
      ray->u =
          1e-3; // ??? which epsilon?  // transport epsilon to postIntersect

      // NOTE(jda) - Initialize ray.Ng with something, even if it's wrong
      //             (better than garbage). Real normals are computed if DG_NS
      //             flag is specified to postIntersect()
      ray->Ng = negate(ray->dir);
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
  Volume *uniform volume = self->volumetricModel->volume;

  if (flags & DG_NS) {
    dg.Ns = Volume_getGradient(volume, dg.P);
    if (dot(dg.Ns, dg.Ns) < 1e-6f)
      dg.Ns = neg(ray.dir); // make_vec3f(1.f,0.f,0.f);
  }

  dg.Ng = dg.Ns;
  // convert ray-space epsilon (in ray.u) to object-/world-space using max(dir)
  // instead of costly length; the error is at most sqrt(3)~1.7, quite
  // acceptable for eps
  dg.epsilon = reduce_max(abs(ray.dir)) * ray.u;

  if (flags & DG_COLOR) {
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
    void *uniform _volume,
    void *uniform _valueSelector)
{
  Isosurfaces *uniform self = (Isosurfaces * uniform) _self;
  VolumetricModel *uniform volume = (VolumetricModel * uniform) _volume;
  VKLValueSelector valueSelector = (VKLValueSelector)_valueSelector;

  self->isovalues = isovalues;
  self->volumetricModel = volume;
  self->super.numPrimitives = 1; // for embree it is just one prim
  self->valueSelector = valueSelector;

  Geometry_setEmbreeUserGeometry(&self->super,
      (RTCGeometry)_embreeGeometry,
      Isosurfaces_bounds,
      Isosurfaces_intersect,
      Isosurfaces_occluded);

  self->super.numPrimitives = numIsovalues;
}
