// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

struct Slices
{
  uniform Geometry super; //!< inherited geometry fields
  uniform vec4f *uniform planes;
  uniform Volume *uniform volume;
};

unmasked void Slices_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  uniform Slices *uniform self = (uniform Slices *uniform)args->geometryUserPtr;
  uniform int primID = args->primID;

  box3fa *uniform out = (box3fa *uniform)args->bounds_o;
  *out = make_box3fa(self->volume->boundingBox);
}

void Slices_intersect_kernel(const RTCIntersectFunctionNArguments *uniform args,
                                const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex]) return;

  uniform Slices *uniform self
    = (uniform Slices *uniform)args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer toa varying ray!
  varying Ray *uniform ray = (varying Ray *uniform)args->rayhit;


  const float tIntersect = intersectPlane(*ray, self->planes[primID]);

  float tBox0, tBox1;
  intersectBox(*ray, self->volume->boundingBox, tBox0, tBox1);

  // slice intersections ignored where NaNs exist in the volume
  bool hit = !isnan(tIntersect) &&
    tIntersect >= max(ray->t0, tBox0) &&
    tIntersect <= min(ray->t, tBox1) &&
    !isnan(self->volume->sample(self->volume, ray->org + tIntersect*ray->dir));

  if (hit) {
    if (isOcclusionTest) {
      ray->t = neg_inf;
    } else {
      ray->instID = args->context->instID[0];
      ray->geomID = self->super.geomID;
      ray->primID = primID;
      ray->t = tIntersect;
      ray->Ng = make_vec3f(self->planes[primID]);
    }
  }
}

unmasked void Slices_intersect(const struct RTCIntersectFunctionNArguments *uniform args)
{
  Slices_intersect_kernel(args,false);
}

unmasked void Slices_occluded(const struct RTCIntersectFunctionNArguments *uniform args)
{
  Slices_intersect_kernel(args,true);
}

static void Slices_postIntersect(uniform Geometry *uniform geometry,
                                 uniform Model *uniform model,
                                 varying DifferentialGeometry &dg,
                                 const varying Ray &ray,
                                 uniform int64 flags)
{
  dg.Ng = dg.Ns = ray.Ng;

  if ((flags & DG_COLOR)) {
    uniform Slices *uniform self = (uniform Slices *uniform)geometry;
    const float sample = self->volume->sample(self->volume, dg.P);
    const vec3f sampleColor = self->volume->transferFunction->getColorForValue(self->volume->transferFunction, sample);
    const float sampleOpacity = 1.f; // later allow "opacity" parameter on slices.

    dg.color = make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, sampleOpacity);
  }
}

export void *uniform Slices_create(void *uniform cppEquivalent)
{
  uniform Slices *uniform self = uniform new uniform Slices;

  Geometry_Constructor(&self->super, cppEquivalent, Slices_postIntersect,
                       NULL, NULL, 0, NULL);

  return self;
}

export void *uniform Slices_set(void          *uniform _self,
                                void          *uniform _model,
                                int32          uniform numPlanes,
                                uniform vec4f *uniform planes,
                                void          *uniform _volume)
{
  uniform Slices *uniform self   = (uniform Slices *uniform)_self;
  uniform Model  *uniform model  = (uniform Model  *uniform)_model;
  uniform Volume *uniform volume = (uniform Volume *uniform)_volume;

  RTCGeometry geom = rtcNewGeometry(ispc_embreeDevice(),RTC_GEOMETRY_TYPE_USER);
  uniform uint32 geomID = rtcAttachGeometry(model->embreeSceneHandle,geom);

  self->super.model  = model;
  self->super.geomID = geomID;
  self->super.numPrimitives = numPlanes;
  self->planes = planes;
  self->volume = volume;

  rtcSetGeometryUserData(geom, self);
  rtcSetGeometryUserPrimitiveCount(geom,numPlanes);
  rtcSetGeometryBoundsFunction
    (geom,(uniform RTCBoundsFunction)&Slices_bounds, self);
  rtcSetGeometryIntersectFunction
    (geom,(uniform RTCIntersectFunctionN)&Slices_intersect);
  rtcSetGeometryOccludedFunction
    (geom,(uniform RTCOccludedFunctionN)&Slices_occluded);
  rtcCommitGeometry(geom);
  rtcReleaseGeometry(geom);
}
