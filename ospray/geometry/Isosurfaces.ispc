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
#include "math/vec.ih"
#include "math/box.ih"
#include "common/Ray.ih"
#include "common/Model.ih"
#include "geometry/Geometry.ih"
#include "volume/Volume.ih"

struct Isosurfaces {
  uniform Geometry super; //!< inherited geometry fields
  uniform float *uniform isovalues;
  uniform Volume *uniform volume;
};

unmasked void Isosurfaces_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  uniform Isosurfaces *uniform self = (uniform Isosurfaces *uniform)args->geometryUserPtr;
  uniform int primID = args->primID;

  box3fa *uniform out = (box3fa *uniform)args->bounds_o;
  *out = make_box3fa(self->volume->boundingBox);
}

void Isosurfaces_intersect_kernel(const RTCIntersectFunctionNArguments *uniform args,
                                const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex]) return;

  uniform Isosurfaces *uniform self
    = (uniform Isosurfaces *uniform)args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer toa varying ray!
  varying Ray *uniform ray = (varying Ray *uniform)args->rayhit;

  uniform Volume *uniform volume = self->volume;

  float hit_t;
  vec3f hit_Ng;

  /* iw - just to explain the following code sequence: in embree3, the
     'occluded' function no longer guarantees that we have a 'full'
     ospray ray to intersect (the ospray ray is what emrbee calls a
     rayhit, the concatenation of ray _and_ hitpoint!), but that
     voluem->intersectIsoSurface code seems to assume that this is
     always a ful ospray::ray. So, just to make sure that this code
     always has a valid "full" ray to work with I'm creating a temp
     one here, and then, when writing results back, make sure that we
     not write any hit information unless it's not an occlusoin
     test */
  Ray tmpRay;
  tmpRay.org = ray->org;
  tmpRay.time = ray->time;
  tmpRay.dir = ray->dir;
  tmpRay.t0 = ray->t0;
  tmpRay.t = ray->t;
  tmpRay.instID = -1;
  tmpRay.geomID = -1;
  tmpRay.primID = -1;

  volume->intersectIsosurface(volume,
                              self->isovalues,
                              self->super.numPrimitives,
                              self->super.geomID,
                              tmpRay);
  const bool hit = tmpRay.primID >= 0;
  if (hit) {
    if (isOcclusionTest) {
      ray->t = neg_inf;
    } else {
      ray->instID = args->context->instID[0];
      ray->geomID = self->super.geomID;
      ray->primID = tmpRay.primID;
      ray->t =  tmpRay.t;
      ray->Ng = tmpRay.Ng;
      ray->u = tmpRay.u; // transport epsilon to postIntersect
    }
  }
}

unmasked void Isosurfaces_intersect(const struct RTCIntersectFunctionNArguments *uniform args)
{
  Isosurfaces_intersect_kernel(args,false);
}

unmasked void Isosurfaces_occluded(const struct RTCIntersectFunctionNArguments *uniform args)
{
  Isosurfaces_intersect_kernel(args,true);
}

static void Isosurfaces_postIntersect(uniform Geometry *uniform geometry,
                                      uniform Model *uniform model,
                                      varying DifferentialGeometry &dg,
                                      const varying Ray &ray,
                                      uniform int64 flags)
{
  uniform Isosurfaces *uniform self = (uniform Isosurfaces *uniform)geometry;

  if (flags & DG_NS) {
    dg.Ns = self->volume->computeGradient(self->volume, dg.P);
    if (dot(dg.Ns,dg.Ns) < 1e-6f)
      dg.Ns = neg(ray.dir); //make_vec3f(1.f,0.f,0.f);
  }

  dg.Ng = dg.Ns;
  // convert ray-space epsilon (in ray.u) to object-/world-space using max(dir)
  // instead of costly length; the error is at most sqrt(3)~1.7, quite
  // acceptable for eps
  dg.epsilon = reduce_max(abs(ray.dir)) * ray.u;

  if (flags & DG_COLOR) {
   TransferFunction *uniform xf = self->volume->transferFunction;
   const vec3f sampleColor = xf->getColorForValue(xf, self->isovalues[ray.primID]);
   const float sampleOpacity = 1.f; // later allow "opacity" parameter on isosurfaces.
   dg.color = make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, sampleOpacity);
  }
}

export void *uniform Isosurfaces_create(void *uniform cppEquivalent)
{
  uniform Isosurfaces *uniform isosurfaces = uniform new uniform Isosurfaces;

  Geometry_Constructor(&isosurfaces->super, cppEquivalent, Isosurfaces_postIntersect, NULL, 0, NULL);

  return isosurfaces;
}

export void *uniform Isosurfaces_set(void          *uniform _self,
                                     void          *uniform _model,
                                     int32          uniform numIsovalues,
                                     uniform float *uniform isovalues,
                                     void          *uniform _volume)
{
  uniform Isosurfaces *uniform self = (uniform Isosurfaces *uniform)_self;
  uniform Model *uniform model = (uniform Model *uniform)_model;
  uniform Volume *uniform volume = (uniform Volume *uniform)_volume;

  RTCGeometry geom = rtcNewGeometry(ispc_embreeDevice(),RTC_GEOMETRY_TYPE_USER);
  uniform uint32 geomID = rtcAttachGeometry(model->embreeSceneHandle,geom);

  self->super.model = model;
  self->super.geomID = geomID;
  self->super.numPrimitives = numIsovalues;
  self->isovalues = isovalues;
  self->volume = volume;

  rtcSetGeometryUserData(geom, self);
  rtcSetGeometryUserPrimitiveCount(geom,1);
  rtcSetGeometryBoundsFunction
    (geom,(uniform RTCBoundsFunction)&Isosurfaces_bounds, self);
  rtcSetGeometryIntersectFunction
    (geom,(uniform RTCIntersectFunctionN)&Isosurfaces_intersect);
  rtcSetGeometryOccludedFunction
    (geom,(uniform RTCOccludedFunctionN)&Isosurfaces_occluded);
  rtcCommitGeometry(geom);
  rtcReleaseGeometry(geom);
}
