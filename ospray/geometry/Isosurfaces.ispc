// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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
#include "ospray/math/vec.ih"
#include "ospray/math/box.ih"
#include "ospray/common/Ray.ih"
#include "ospray/common/Model.ih"
#include "ospray/geometry/Geometry.ih"
#include "ospray/volume/Volume.ih"
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_geometry_user.isph"

struct Isosurfaces {
  uniform Geometry geometry; //!< inherited geometry fields
  uniform int32 numIsovalues;
  uniform float *uniform isovalues;
  uniform Volume *uniform volume;
};

void Isosurfaces_bounds(uniform Isosurfaces *uniform isosurfaces,
                        uniform size_t primID,
                        uniform box3fa &bbox)
{
  bbox = make_box3fa(isosurfaces->volume->boundingBox);
}

void Isosurfaces_intersect(uniform Isosurfaces *uniform self,
                           varying Ray &ray,
                           uniform size_t primID)
{
  // Volume of interest.
  uniform Volume *uniform volume = self->volume;

  // Operate on a copy of the ray.
  Ray rayCopy = ray;
  rayCopy.t = ray.t0 + ray.time; // use ray.time as a ray offset
  rayCopy.t1 = ray.t;

  // Sample the volume at the current point in world coordinates.
  float t0 = rayCopy.t;
  float sample0 = volume->computeSample(volume, rayCopy.org + rayCopy.t * rayCopy.dir);

  while(1) {

    // Advance the ray.
    volume->intersectIsosurface(volume, self->isovalues, self->numIsovalues, rayCopy);

    if (rayCopy.t > rayCopy.t1)
      return;

    const float t = rayCopy.t;
    const float sample = volume->computeSample(volume, rayCopy.org + rayCopy.t * rayCopy.dir);

    // Find t value and isovalue index for the first isosurface intersection.
    float tHit = infinity;
    int isovalueID;

    if (!isnan(sample0+sample)) {
      for (uniform int i=0; i<self->numIsovalues; i++) {
        if ((self->isovalues[i] - sample0) * (self->isovalues[i] - sample) <= 0.f) {
          const float tIso = t0 + (self->isovalues[i] - sample0) * rcpf(sample - sample0) * (t - t0);

          if (tIso < tHit) {
            tHit = tIso;
            isovalueID = i;
          }
        }
      }

      // If we found a hit, update ray information and return.
      if (tHit <= rayCopy.t1) {
        ray.geomID = self->geometry.geomID;
        ray.primID = isovalueID;
        ray.t = tHit;
        return;
      }
    }

    t0 = t;
    sample0 = sample;
  }
}

static void Isosurfaces_postIntersect(uniform Geometry *uniform geometry,
                                      uniform Model *uniform model,
                                      varying DifferentialGeometry &dg,
                                      const varying Ray &ray,
                                      uniform int64 flags)
{
  uniform Isosurfaces *uniform self = (uniform Isosurfaces *uniform)geometry;

  if ((flags & DG_NS))
    dg.Ns = self->volume->computeGradient(self->volume, dg.P);

  if ((flags & DG_COLOR)) {
    const vec3f sampleColor = self->volume->transferFunction->getColorForValue(self->volume->transferFunction, self->isovalues[ray.primID]);
    const float sampleOpacity = 1.f; // later allow "opacity" parameter on isosurfaces.
    dg.color = make_vec4f(sampleColor.x, sampleColor.y, sampleColor.z, sampleOpacity);
  }
}

export void *uniform Isosurfaces_create(void *uniform cppEquivalent)
{
  uniform Isosurfaces *uniform isosurfaces = uniform new uniform Isosurfaces;

  Geometry_Constructor(&isosurfaces->geometry, cppEquivalent, Isosurfaces_postIntersect, NULL, 0, NULL);

  return isosurfaces;
}

export void *uniform Isosurfaces_set(void          *uniform _isosurfaces,
                                     void          *uniform _model,
                                     int32          uniform numIsovalues,
                                     uniform float *uniform isovalues,
                                     void          *uniform _volume)
{
  uniform Isosurfaces *uniform isosurfaces = (uniform Isosurfaces *uniform)_isosurfaces;
  uniform Model *uniform model = (uniform Model *uniform)_model;
  uniform Volume *uniform volume = (uniform Volume *uniform)_volume;

  uniform uint32 geomID = rtcNewUserGeometry(model->embreeSceneHandle, 1);

  isosurfaces->geometry.model = model;
  isosurfaces->geometry.geomID = geomID;
  isosurfaces->numIsovalues = numIsovalues;
  isosurfaces->isovalues = isovalues;
  isosurfaces->volume = volume;

  rtcSetUserData(model->embreeSceneHandle, geomID, isosurfaces);
  rtcSetBoundsFunction(model->embreeSceneHandle, geomID, (uniform RTCBoundsFunc)&Isosurfaces_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle, geomID, (uniform RTCIntersectFuncVarying)&Isosurfaces_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle, geomID, (uniform RTCOccludedFuncVarying)&Isosurfaces_intersect);
}
