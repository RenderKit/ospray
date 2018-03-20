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
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_geometry_user.isph"

struct Isosurfaces {
  uniform Geometry super; //!< inherited geometry fields
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
  uniform Volume *uniform volume = self->volume;

  volume->intersectIsosurface(volume,
                              self->isovalues,
                              self->super.numPrimitives,
                              self->super.geomID,
                              ray);
}

static void Isosurfaces_postIntersect(uniform Geometry *uniform geometry,
                                      uniform Model *uniform model,
                                      varying DifferentialGeometry &dg,
                                      const varying Ray &ray,
                                      uniform int64 flags)
{
  uniform Isosurfaces *uniform self = (uniform Isosurfaces *uniform)geometry;

  if ((flags & DG_NS)) {
    dg.Ns = self->volume->computeGradient(self->volume, dg.P);
    if (dot(dg.Ns,dg.Ns) < 1e-6f)
      dg.Ns = neg(ray.dir); //make_vec3f(1.f,0.f,0.f);
  }

  dg.Ng = dg.Ns;

  if ((flags & DG_COLOR)) {
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

  isosurfaces->super.model = model;
  isosurfaces->super.geomID = geomID;
  isosurfaces->super.numPrimitives = numIsovalues;
  isosurfaces->isovalues = isovalues;
  isosurfaces->volume = volume;

  rtcSetUserData(model->embreeSceneHandle, geomID, isosurfaces);
  rtcSetBoundsFunction(model->embreeSceneHandle, geomID, (uniform RTCBoundsFunc)&Isosurfaces_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle, geomID, (uniform RTCIntersectFuncVarying)&Isosurfaces_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle, geomID, (uniform RTCOccludedFuncVarying)&Isosurfaces_intersect);
}
