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
#include "Subdivision.ih"
#include "common/Model.ih"
#include "common/safe_gather.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"

int32 Subdivision_getMaterialID(
    const Geometry *uniform const _self
    , const int32 primID
    )
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
  //TODO: grab normal from embree intersect
}

void Subdivision_Constructor(uniform Subdivision *uniform mesh,
                             void *uniform cppEquivalent,
                             uniform Model *uniform model,
                             RTCGeometry geom,
                             uniform int32 geomID,
                             uniform int32   geom_materialID,
                             uniform uint32 *uniform prim_materialID,
                             uniform Material *uniform *uniform materialList)
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
}


export void *uniform Subdivision_create(void *uniform cppEquivalent)
{
  Subdivision *uniform mesh = uniform new Subdivision;
  return mesh;
}

export void *uniform Subdivision_set(void *uniform _mesh,
                                      void *uniform _model,
                                      RTCGeometry geom,
                                      uniform int32 geomID,
                                      uniform int32 geom_materialID,
                                      uniform uint32 *uniform prim_materialID,
                                      void *uniform _materialList
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
                          (Material*uniform*uniform)_materialList);
}
