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
#include "Subdivision.ih"
#include "common/Model.ih"
#include "common/safe_gather.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"

int32 Subdivision_getMaterialID(
    const Geometry *uniform const _self, const int32 primID)
{
  // TODO: gather material ID from index array if indexSize==4
  const Subdivision *const uniform self = (const Subdivision *uniform)_self;
  return self->prim_materialID ? self->prim_materialID[primID] : self->geom_materialID;
}

static void Subdivision_postIntersect(uniform Geometry *uniform _self,
                                       uniform Model    *uniform model,
                                       varying DifferentialGeometry &dg,
                                       const varying Ray &ray,
                                       uniform int64 flags)
{
  Subdivision *uniform self = (Subdivision *uniform)_self;
  dg.Ng = dg.Ns = ray.Ng;
  if (flags & DG_MATERIALID) {
    dg.materialID = Subdivision_getMaterialID(_self, ray.primID);
  }
  //get normals
  if (flags & DG_NS) {
    vec3f dPdu,dPdv;
    foreach_unique (geomID in ray.geomID) {
      rtcInterpolateV1(rtcGetGeometry(model->embreeSceneHandle,geomID),ray.primID,
                       ray.u,ray.v,RTC_BUFFER_TYPE_VERTEX,0,NULL,&dPdu.x,&dPdv.x,3);
    }
    dg.Ns = cross(dPdu,dPdv);
  }
  //get colors
  if (flags & DG_COLOR) {
    foreach_unique (geomID in ray.geomID) {
      rtcInterpolateV1(rtcGetGeometry(model->embreeSceneHandle,geomID),ray.primID,
                       ray.u,ray.v,RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,0,&dg.color.x,NULL,NULL,4);
    }
  }
  //get texcoords
  if (flags & DG_TEXCOORD && self->texcoord) {
    foreach_unique (geomID in ray.geomID) {
      rtcInterpolateV1(rtcGetGeometry(model->embreeSceneHandle,geomID),ray.primID,
                       ray.u,ray.v,RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,1,&dg.st.x,NULL,NULL,2);
    }
  }
}

void Subdivision_Constructor(uniform Subdivision *uniform mesh,
                             void *uniform cppEquivalent,
                             uniform Model *uniform model,
                             RTCGeometry geom,
                             uniform int32 geomID,
                             uniform int32   geom_materialID,
                             uniform uint32 *uniform prim_materialID,
                             uniform Material *uniform *uniform materialList,
                             uniform vec2f *uniform texcoord)
{
  Geometry_Constructor(&mesh->super,cppEquivalent,
                       Subdivision_postIntersect,
                       model,
                       geom,
                       geomID,
                       materialList);
  mesh->super.getMaterialID = Subdivision_getMaterialID;
  mesh->prim_materialID = prim_materialID;
  mesh->geom_materialID = geom_materialID;
  mesh->texcoord = texcoord;
}


export void *uniform Subdivision_create(void *uniform cppEquivalent)
{
  Subdivision *uniform mesh = uniform new Subdivision;
  Subdivision_Constructor(mesh,
                          cppEquivalent,
                          NULL,
                          NULL,
                          0,
                          0,
                          0,
                          NULL,
                          NULL);
  return mesh;
}

export void *uniform Subdivision_set(void *uniform _mesh,
                                      void *uniform _model,
                                      RTCGeometry geom,
                                      uniform int32 geomID,
                                      uniform int32 geom_materialID,
                                      uniform uint32 *uniform prim_materialID,
                                      void *uniform _materialList,
                                      uniform vec2f *uniform texcoord
                                      )
{
  uniform Subdivision *uniform mesh = (uniform Subdivision *uniform)_mesh;
  uniform Model *uniform model = (uniform Model *uniform)_model;
  Subdivision_Constructor(mesh,
                          mesh->super.cppEquivalent,
                          model,
                          geom,
                          geomID,
                          geom_materialID,
                          prim_materialID,
                          (Material*uniform*uniform)_materialList,
                          texcoord
                          );
}
