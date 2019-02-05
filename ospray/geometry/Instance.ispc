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
#include "Instance.ih"

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

    instancedGeometry->postIntersect(instancedGeometry,instancedModel,
                                     dg,ray,flags);

    if (instancedGeometry->materialList) {
      dg.material = instancedGeometry->materialList[dg.materialID < 0 ? 0 : dg.materialID];
    }
  }
  dg.Ns = xfmVector(transposed(self->rcp_xfm.l), dg.Ns);
  dg.Ng = xfmVector(transposed(self->rcp_xfm.l), dg.Ng);
  // scale dg.epsilon by max (epsilon is scalar and thus assumed to be
  // isotropic anyway and hence cannot better handle non-uniform scaling)
  dg.epsilon *= max(abs(self->xfm.l.vx.x),
      max(abs(self->xfm.l.vy.y), abs(self->xfm.l.vz.z)));

  if (flags & DG_TANGENTS) {
    dg.dPds = xfmVector(self->xfm,dg.dPds);
    dg.dPdt = xfmVector(self->xfm,dg.dPdt);
  }
}

export void *uniform InstanceGeometry_create(void *uniform cppE)
{
  Instance *uniform self = uniform new Instance;
  Geometry_Constructor(&self->super, cppE,
                       Instance_postIntersect,
                       NULL,
                       NULL,
                       -1,
                       NULL);
  self->areaPDF = NULL;

  return self;
}

export void InstanceGeometry_set(void *uniform _self,
                                 const uniform AffineSpace3f &xfm,
                                 const uniform AffineSpace3f &rcp_xfm,
                                 void *uniform _model,
                                 float *uniform areaPDF)
{
  Instance *uniform self = (Instance *uniform)_self;
  self->model   = (uniform Model *uniform)_model;
  self->xfm     = xfm;
  self->rcp_xfm = rcp_xfm;
  self->areaPDF = areaPDF;
}
