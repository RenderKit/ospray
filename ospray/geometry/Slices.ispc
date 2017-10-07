// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_geometry_user.isph"

struct Slices {
  uniform Geometry super; //!< inherited geometry fields
  uniform vec4f *uniform planes;
  uniform Volume *uniform volume;
};

void Slices_bounds(uniform Slices *uniform slices,
                   uniform size_t primID,
                   uniform box3fa &bbox)
{
  bbox = make_box3fa(slices->volume->boundingBox);
}

void Slices_intersect(uniform Slices *uniform slices,
                      varying Ray &ray,
                      uniform size_t primID)
{
  const float tIntersect = intersectPlane(ray, slices->planes[primID]);

  float tBox0, tBox1;
  intersectBox(ray, slices->volume->boundingBox, tBox0, tBox1);

  // slice intersections ignored where NaNs exist in the volume
  if (!isnan(tIntersect) && tIntersect >= max(ray.t0, tBox0) && tIntersect <= min(ray.t, tBox1) &&
      !isnan(slices->volume->sample(slices->volume, ray.org + tIntersect*ray.dir))) {
    ray.geomID = slices->super.geomID;
    ray.primID = primID;
    ray.t = tIntersect;
    ray.Ng = make_vec3f(slices->planes[primID]);
  }
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
  uniform Slices *uniform slices = uniform new uniform Slices;

  Geometry_Constructor(&slices->super, cppEquivalent, Slices_postIntersect, NULL, 0, NULL);

  return slices;
}

export void *uniform Slices_set(void          *uniform _slices,
                                void          *uniform _model,
                                int32          uniform numPlanes,
                                uniform vec4f *uniform planes,
                                void          *uniform _volume)
{
  uniform Slices *uniform slices = (uniform Slices *uniform)_slices;
  uniform Model *uniform model = (uniform Model *uniform)_model;
  uniform Volume *uniform volume = (uniform Volume *uniform)_volume;

  uniform uint32 geomID = rtcNewUserGeometry(model->embreeSceneHandle, numPlanes);

  slices->super.model  = model;
  slices->super.geomID = geomID;
  slices->super.primitives = numPlanes;
  slices->planes = planes;
  slices->volume = volume;

  rtcSetUserData(model->embreeSceneHandle, geomID, slices);
  rtcSetBoundsFunction(model->embreeSceneHandle, geomID, (uniform RTCBoundsFunc)&Slices_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle, geomID, (uniform RTCIntersectFuncVarying)&Slices_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle, geomID, (uniform RTCOccludedFuncVarying)&Slices_intersect);
}
