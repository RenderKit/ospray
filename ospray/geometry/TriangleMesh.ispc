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
#include "TriangleMesh.ih"
#include "ospray/common/Model.ih"
// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_geometry.isph"
#include "embree2/rtcore_geometry_user.isph"

static void TriangleMesh_postIntersect(uniform Geometry *uniform _self,
                                       uniform Model    *uniform model,
                                       varying DifferentialGeometry &dg,
                                       const varying Ray &ray,
                                       uniform int64 flags)
{
  uniform TriangleMesh *uniform self = (uniform TriangleMesh *uniform)_self;
  dg.Ng = dg.Ns = ray.Ng;
  
  const varying int indexBase = self->idxSize * ray.primID;
  const varying vec3i index = make_vec3i(self->index[indexBase+0],
                                         self->index[indexBase+1],
                                         self->index[indexBase+2]);
  uniform float *uniform normal = self->normal;
  if ((flags & DG_NS) && normal) {
    const uniform vec3f *const varying n0ptr = (const uniform vec3f *)&normal[index.x * self->norSize];
    const uniform vec3f *const varying n1ptr = (const uniform vec3f *)&normal[index.y * self->norSize];
    const uniform vec3f *const varying n2ptr = (const uniform vec3f *)&normal[index.z * self->norSize];
    dg.Ns
      = (1.f-ray.u-ray.v) * *n0ptr
      + ray.u * *n1ptr
      + ray.v * *n2ptr;
  }

  if ((flags & DG_COLOR)) {
    uniform vec4f *uniform color = self->color;
    if (color) {
      dg.color
        = (1.f-ray.u-ray.v) * (color[index.x])
        + ray.u * (color[index.y])
        + ray.v * (color[index.z]);
    }
  }

  if (flags & DG_TEXCOORD && self->texcoord) {
    //calculate texture coordinate using barycentric coordinates
    const uniform vec2f  *uniform texcoord = self->texcoord;
    dg.st
      = (1.f-ray.u-ray.v) * (texcoord[index.x])
      + ray.u * (texcoord[index.y])
      + ray.v * (texcoord[index.z]);
  } else {
    dg.st = make_vec2f(0.0f, 0.0f);
  }

  if (flags & DG_MATERIALID) {
    if (self->prim_materialID) {
      dg.materialID = self->prim_materialID[ray.primID];
    } 
    // TODO: gather material ID from index array if indexSize==4
    else {
      dg.materialID = self->geom_materialID;
    }

    if( self->materialList)
      dg.material = self->materialList[dg.materialID < 0 ? 0 : dg.materialID];
  }
}


//! constructor for ispc-side TriangleMesh object
extern void TriangleMesh_Constructor(uniform TriangleMesh *uniform mesh,
                                     void *uniform cppEquivalent,
                                     uniform Model *uniform model,
                                     uniform int32  geomID,
                                     uniform int32  numTriangles,
                                     uniform int32  idxSize,
                                     uniform int32  norSize,
                                     uniform int    *uniform index,
                                     uniform float  *uniform normal,
                                     uniform vec4f  *uniform color,
                                     uniform vec2f  *uniform texcoord,
                                     uniform int32   geom_materialID,
                                     uniform Material *uniform material,
                                     uniform Material *uniform *uniform materialList,
                                     uniform uint32 *uniform prim_materialID
                                     )
{
  Geometry_Constructor(&mesh->geometry,cppEquivalent,
                       TriangleMesh_postIntersect,
                       model,geomID,
                       material);
  mesh->numTriangles = numTriangles;
  mesh->index        = index;
  mesh->normal       = normal;
  mesh->color        = color;
  mesh->texcoord     = texcoord;
  mesh->idxSize      = idxSize;
  mesh->norSize      = norSize;
  mesh->prim_materialID = prim_materialID;
  mesh->materialList = materialList;
  mesh->geom_materialID = geom_materialID;
}

#ifdef OSPRAY_INTERSECTION_FILTER
static void intersectionFilter(void* uniform ptr,   /*!< pointer to user data */
                               varying Ray &ray  /*!< intersection to filter */)
{
  uniform Geometry *uniform geom = (uniform Geometry *uniform)ptr;
  if (ray.intersectionFilter) {
    ray.intersectionFilter(geom,(varying Ray &)ray);
  }
}
#endif


//! destructor for ispc-side TriangleMesh object
extern void TriangleMesh_Destructor(uniform TriangleMesh *uniform mesh)
{ 
  Geometry_Destructor(&mesh->geometry); 
}

//! free and properly destroy the given mesh geometry
export void TriangleMesh_destroy(void *uniform _mesh)
{
  uniform TriangleMesh *uniform mesh = (uniform TriangleMesh *uniform)_mesh;
  TriangleMesh_Destructor(mesh);
  delete mesh;
}

export void *uniform TriangleMesh_create(void *uniform cppEquivalent)
{
  uniform TriangleMesh *uniform mesh = uniform new uniform TriangleMesh;
  TriangleMesh_Constructor(mesh, cppEquivalent, 
                           NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL, -1, NULL, NULL, NULL);
  return mesh;
}

export void *uniform TriangleMesh_set(void *uniform _mesh,
                                      void *uniform _model,
                                      uniform int32  geomID,
                                      uniform int32  numTriangles,
                                      uniform int32  idxSize,
                                      uniform int32  norSize,
                                      uniform int    *uniform index,
                                      uniform float  *uniform normal,
                                      uniform vec4f  *uniform color,
                                      uniform vec2f  *uniform texcoord,
                                      uniform int32   geom_materialID,
                                      void *uniform material,
                                      void *uniform _materialList,
                                      uniform uint32 *uniform prim_materialID
                                      )
{
  uniform TriangleMesh *uniform mesh = (uniform TriangleMesh *uniform)_mesh;
  uniform Model *uniform model = (uniform Model *uniform)_model;
  uniform Material *uniform *uniform materialList = (uniform Material *uniform *uniform)_materialList;
  TriangleMesh_Constructor(mesh,
                           mesh->geometry.cppEquivalent,
                           model,
                           geomID, 
                           numTriangles,
                           idxSize,norSize,
                           index,
                           normal,
                           color, 
                           texcoord,
                           geom_materialID,
                           (Material*uniform)material,
                           (Material*uniform*uniform)materialList,
                           prim_materialID);
#ifdef OSPRAY_INTERSECTION_FILTER
 rtcSetUserData(model->embreeSceneHandle,geomID,mesh);
 rtcSetIntersectionFilterFunction(model->embreeSceneHandle,geomID,
                                  (uniform RTCFilterFuncVarying)&intersectionFilter);
#endif
}

