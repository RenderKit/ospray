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

#include "AMRVolume.ih"
#include "../amr/AMR.ih"
// ospray
#include "ospray/math/AffineSpace.ih"
#include "ospray/common/Ray.ih"
#include "ospray/geometry/Geometry.ih"
#include "ospray/common/Model.ih"
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_geometry_user.isph"

struct AMRGeometry
{
  Geometry   super;         //!< inherited geometry fields
  AMR       *amr;        //!< pointer to the amr volume that we traverse
  AMRVolume *amrVolume;
  /*! @{ the iso-values we are going to be looking for */
  const float  *isoValue;
  uint32        numIsoValues;
  /*! @} */

  /*! @{ the list of leaves active for this iso-value list */
  const AMRLeaf  **activeLeaf;
  uint32           numActiveLeaves;
  /*! @} */
};

void AMRGeometry_postIntersect(uniform AMRGeometry *uniform self,
                                  uniform Model *uniform model,
                                  varying DifferentialGeometry &dg,
                                  const varying Ray &ray,
                                  uniform int64 flags)
{
  dg.Ng = dg.Ns = ray.Ng;
  dg.material = self->super.material;
}

export void *uniform AMRGeometry_create(void *uniform cppEquivalent)
{
  uniform AMRGeometry *uniform self = uniform new uniform AMRGeometry;
  Geometry_Constructor(&self->super,cppEquivalent,
                       (Geometry_postIntersectFct)&AMRGeometry_postIntersect,
                       NULL,0,NULL);
  self->isoValue = NULL;
  self->numIsoValues = 0;
  self->activeLeaf = NULL;
  self->numActiveLeaves = 0;
  return self;
}

unmasked void AMRGeometry_bounds(uniform AMRGeometry *uniform self,
                     uniform size_t primID,
                     uniform box3fa &bbox)
{
  bbox = make_box3fa(self->activeLeaf[primID]->bounds);
}

void AMRGeometry_intersect(uniform AMRGeometry *uniform self,
               varying Ray &ray,
               uniform size_t primID)
{
  const AMRLeaf *uniform leaf = self->activeLeaf[primID];
  float t0, t1;
  intersectBox(ray,leaf->bounds,t0,t1);
  if (t0 > t1) return;
  ray.primID = primID;
  ray.geomID = self->super.geomID;
  ray.t = t0;
  ray.Ng = neg(ray.dir);
}

export void AMRGeometry_set(void   *uniform _self,
                               void   *uniform _model,
                               void   *uniform _amrVolume,
                               void   *uniform activeLeaf,
                               uint32  uniform numActiveLeaves,
                               const float  *uniform isoValue,
                               uint32  uniform numIsoValues)
{
  AMRGeometry *uniform self = (AMRGeometry *uniform)_self;
  Model *uniform model = (Model *uniform)_model;

  uniform uint32 geomID = rtcNewUserGeometry(model->embreeSceneHandle,numActiveLeaves);

  self->super.model     = model;
  self->super.geomID    = geomID;
  self->isoValue        = isoValue;
  self->numIsoValues    = numIsoValues;
  self->activeLeaf      = (const AMRLeaf **uniform)activeLeaf;
  self->numActiveLeaves = numActiveLeaves;

  rtcSetUserData(model->embreeSceneHandle,geomID,self);
  rtcSetBoundsFunction(model->embreeSceneHandle,geomID,
                       (uniform RTCBoundsFunc)&AMRGeometry_bounds);
  rtcSetIntersectFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCIntersectFuncVarying)&AMRGeometry_intersect);
  rtcSetOccludedFunction(model->embreeSceneHandle,geomID,
                          (uniform RTCOccludedFuncVarying)&AMRGeometry_intersect);

}