// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// ospray
#include "common/Ray.ih"
#include "common/World.ih"
#include "geometry/Geometry.ih"
#include "math/box.ih"
#include "math/vec.ih"
#include "volume/Volume.ih"

struct Isosurfaces
{
  Geometry super;  //!< inherited geometry fields
  float *isovalues;
  VolumeInstance *volumeInstance;
};

unmasked void Isosurfaces_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  uniform Isosurfaces *uniform self =
      (uniform Isosurfaces * uniform) args->geometryUserPtr;
  uniform int primID = args->primID;

  box3fa *uniform out = (box3fa * uniform) args->bounds_o;
  *out                = make_box3fa(self->volumeInstance->volume->boundingBox);
}

void Isosurfaces_intersect_kernel(const RTCIntersectFunctionNArguments *uniform
                                      args,
                                  const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex])
    return;

  Isosurfaces *uniform self   = (Isosurfaces * uniform) args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer toa varying ray!
  varying Ray *uniform ray = (varying Ray * uniform) args->rayhit;

  Volume *uniform volume = self->volumeInstance->volume;

  float hit_t;
  vec3f hit_Ng;

  /* iw - just to explain the following code sequence: in embree3, the
     'occluded' function no longer guarantees that we have a 'full'
     ospray ray to intersect (the ospray ray is what emrbee calls a
     rayhit, the concatenation of ray _and_ hitpoint!), but that
     voluem->intersectIsoSurface code seems to assume that this is
     always a full ospray::Ray. So, just to make sure that this code
     always has a valid "full" ray to work with I'm creating a temp
     one here, and then, when writing results back, make sure that we
     not write any hit information unless it's not an occlusoin
     test */
  Ray tmpRay;

  // NOTE(jda) - SUPER nasty pointer aliasing bug here. If we don't use setRay()
  //             here,
  setRay(tmpRay, ray->org, ray->dir, ray->t0, ray->t);

  volume->intersectIsosurface(volume,
                              self->isovalues,
                              self->super.numPrimitives,
                              self->super.geomID,
                              tmpRay);

  if (hadHit(tmpRay)) {
    if (isOcclusionTest) {
      ray->t = neg_inf;
    } else {
      ray->instID = args->context->instID[0];
      ray->geomID = self->super.geomID;
      ray->primID = tmpRay.primID;
      ray->t      = tmpRay.t;
      ray->u      = tmpRay.u;  // transport epsilon to postIntersect

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
    const struct RTCIntersectFunctionNArguments *uniform args)
{
  Isosurfaces_intersect_kernel(args, true);
}

static void Isosurfaces_postIntersect(const Geometry *uniform geometry,
                                      const World *uniform model,
                                      varying DifferentialGeometry &dg,
                                      const varying Ray &ray,
                                      uniform int64 flags)
{
  Isosurfaces *uniform self = (Isosurfaces * uniform) geometry;
  Volume *uniform volume    = self->volumeInstance->volume;

  if (flags & DG_NS) {
    dg.Ns = volume->computeGradient(volume, dg.P);
    if (dot(dg.Ns, dg.Ns) < 1e-6f)
      dg.Ns = neg(ray.dir);  // make_vec3f(1.f,0.f,0.f);
  }

  dg.Ng = dg.Ns;
  // convert ray-space epsilon (in ray.u) to object-/world-space using max(dir)
  // instead of costly length; the error is at most sqrt(3)~1.7, quite
  // acceptable for eps
  dg.epsilon = reduce_max(abs(ray.dir)) * ray.u;

  if (flags & DG_COLOR) {
    TransferFunction *uniform xf = self->volumeInstance->transferFunction;
    const vec3f sampleColor =
        xf->getColorForValue(xf, self->isovalues[ray.primID]);
    dg.color = make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, 1.f);
  }
}

export void *uniform Isosurfaces_create(void *uniform cppEquivalent)
{
  uniform Isosurfaces *uniform isosurfaces = uniform new uniform Isosurfaces;

  Geometry_Constructor(
      &isosurfaces->super, cppEquivalent, Isosurfaces_postIntersect, NULL, 0);

  return isosurfaces;
}

export void *uniform Isosurfaces_set(void *uniform _self,
                                     void *uniform _embreeGeometry,
                                     uniform uint32 geomID,
                                     int32 uniform numIsovalues,
                                     uniform float *uniform isovalues,
                                     void *uniform _volume)
{
  Isosurfaces *uniform self      = (Isosurfaces * uniform) _self;
  VolumeInstance *uniform volume = (VolumeInstance * uniform) _volume;

  self->super.geomID        = geomID;
  self->super.numPrimitives = numIsovalues;
  self->isovalues           = isovalues;
  self->volumeInstance      = volume;

  RTCGeometry geom = (RTCGeometry)_embreeGeometry;

  rtcSetGeometryUserData(geom, self);
  rtcSetGeometryUserPrimitiveCount(geom, 1);
  rtcSetGeometryBoundsFunction(
      geom, (uniform RTCBoundsFunction)&Isosurfaces_bounds, self);
  rtcSetGeometryIntersectFunction(
      geom, (uniform RTCIntersectFunctionN)&Isosurfaces_intersect);
  rtcSetGeometryOccludedFunction(
      geom, (uniform RTCOccludedFunctionN)&Isosurfaces_occluded);
  rtcCommitGeometry(geom);
}
