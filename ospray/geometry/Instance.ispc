// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
#include "math/AffineSpace.ih"
#include "common/Ray.ih"
#include "common/Model.ih"
#include "geometry/Geometry.ih"
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_geometry_user.isph"

struct Instance {
  uniform Geometry geometry; //!< inherited geometry fields
  uniform Model *uniform model; //!< instanced model
  AffineSpace3f          xfm;   //!< instantiation matrix
  AffineSpace3f          rcp_xfm;   //!< rcp(instantiation matrix)
};



static void Instance_postIntersect(uniform Geometry *uniform _self,
                                   uniform Model *uniform parentModel,
                                   varying DifferentialGeometry &dg,
                                   const varying Ray &ray,
                                   uniform int64 flags)
{
  uniform Instance *uniform self = (uniform Instance *uniform)_self;
  uniform Model *uniform instancedModel = 
    (uniform Model *uniform)self->model;
  foreach_unique(geomID in ray.geomID) {
    uniform Geometry *uniform instancedGeometry = 
      (uniform Geometry *uniform)instancedModel->geometry[geomID];

    dg.geometry = instancedGeometry;
    dg.material = instancedGeometry->material;

    instancedGeometry->postIntersect(instancedGeometry,instancedModel,
                                     dg,ray,flags);
  }
  // XXX this is wrong when the transformation contains non-uniform scaling!
  dg.Ns = xfmVector(self->xfm,dg.Ns);
  dg.Ng = xfmVector(self->xfm,dg.Ng);

  if (flags & DG_TANGENTS) {
    dg.dPds = xfmVector(self->xfm,dg.dPds);
    dg.dPdt = xfmVector(self->xfm,dg.dPdt);
  }
}

export void *uniform InstanceGeometry_create(void *uniform cppE)
{
  uniform Instance *uniform inst = uniform new uniform Instance;
  inst->geometry.cppEquivalent = cppE;
  inst->geometry.postIntersect = Instance_postIntersect;

  return inst;
}

export void InstanceGeometry_set(void *uniform _self, 
                                 const uniform AffineSpace3f &xfm,
                                 const uniform AffineSpace3f &rcp_xfm,
                                 void *uniform _model)
{
  uniform Instance *uniform self = (uniform Instance *uniform)_self;
  self->model   = (uniform Model *uniform)_model;
  self->xfm     = xfm;
  self->rcp_xfm = xfm;
}
