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
#include "TriangleMesh.ih"
#include "ospray/common/Model.ih"
#include "ospray/math/LinearSpace.ih"

// embree
#include "embree2/rtcore.isph"
#include "embree2/rtcore_geometry.isph"
#include "embree2/rtcore_geometry_user.isph"

#define BITS 20

inline vec3i gather64_vec3i(const int *uniform const base, 
                            const uniform int stride, 
                            const varying int index)
{
  vec3i v;

  // special 64-bit safe code:
  const int index_lo = index & ((1<<BITS)-1);
  const int index_hi = index - index_lo;
  // varying offset of lower bits:
  const varying int scaledIndexLo = stride * index_lo;
  foreach_unique(hi in index_hi) {
    //uniform offset for upper bits:
    uniform int64 scaledIndexHi = (int64)hi * (int64)stride;
    // properly shifted base address (shifted by 64-bits)
    const int *uniform base_hi = base + scaledIndexHi;
    
    v.x = base_hi[scaledIndexLo+0];
    v.y = base_hi[scaledIndexLo+1];
    v.z = base_hi[scaledIndexLo+2];
  }
  return v;
}

inline vec3f gather64_vec3f(const float *uniform const base, 
                            const uniform int stride, 
                            const varying int index)
{
  vec3f v;

  // special 64-bit safe code:
  const int index_lo = index & ((1<<BITS)-1);
  const int index_hi = index - index_lo;
  // varying offset of lower bits:
  const varying int scaledIndexLo = stride * index_lo;
  foreach_unique(hi in index_hi) {
    //uniform offset for upper bits:
    uniform int64 scaledIndexHi = (int64)hi * (int64)stride;
    // properly shifted base address (shifted by 64-bits)
    const float *uniform base_hi = base + scaledIndexHi;
    
    v.x = base_hi[scaledIndexLo+0];
    v.y = base_hi[scaledIndexLo+1];
    v.z = base_hi[scaledIndexLo+2];
  }
  return v;
}

inline vec4f gather64_vec4f(const vec4f *uniform const base, 
                            const varying int index)
{
  vec4f v;

  // special 64-bit safe code:
  const int index_lo = index & ((1<<BITS)-1);
  const int index_hi = index - index_lo;
  // varying offset of lower bits:
  const varying int scaledIndexLo = index_lo;
  foreach_unique(hi in index_hi) {
    //uniform offset for upper bits:
    uniform int64 scaledIndexHi = (int64)hi;
    // properly shifted base address (shifted by 64-bits)
    const vec4f *uniform base_hi = base + scaledIndexHi;
    v = base_hi[scaledIndexLo];
  }
  return v;
}

static void TriangleMesh_postIntersect(uniform Geometry *uniform _self,
                                       uniform Model    *uniform model,
                                       varying DifferentialGeometry &dg,
                                       const varying Ray &ray,
                                       uniform int64 flags)
{
  uniform TriangleMesh *uniform self = (uniform TriangleMesh *uniform)_self;
  dg.Ng = dg.Ns = ray.Ng;
#if 1
  const varying vec3i index = gather64_vec3i(self->index,self->idxSize,ray.primID);
#else
  const varying int indexBase = self->idxSize * ray.primID;
  const varying vec3i index = make_vec3i(self->index[indexBase+0],
                                         self->index[indexBase+1],
                                         self->index[indexBase+2]);
#endif

  const uniform float *uniform normal = self->normal;
  const uniform int32 norSize = self->norSize;
  if ((flags & DG_NS) && normal) {
#if 1
    dg.Ns
      = (1.f-ray.u-ray.v) * gather64_vec3f(normal,self->norSize,index.x)
      + ray.u * gather64_vec3f(normal,self->norSize,index.y)
      + ray.v * gather64_vec3f(normal,self->norSize,index.z);
#else
    const uniform vec3f *const varying n0ptr = (const uniform vec3f *)&normal[index.x * norSize];
    const uniform vec3f *const varying n1ptr = (const uniform vec3f *)&normal[index.y * norSize];
    const uniform vec3f *const varying n2ptr = (const uniform vec3f *)&normal[index.z * norSize];
    dg.Ns
      = (1.f-ray.u-ray.v) * *n0ptr
      + ray.u * *n1ptr
      + ray.v * *n2ptr;
#endif
  }

  if ((flags & DG_COLOR)) {
    uniform vec4f *uniform color = self->color;
    if (color) {

#if 1
      dg.color
        = (1.f-ray.u-ray.v) * gather64_vec4f(color,index.x)
        + ray.u * gather64_vec4f(color,index.y)
        + ray.v * gather64_vec4f(color,index.z);
#else
      dg.color
        = (1.f-ray.u-ray.v) * (color[index.x])
        + ray.u * (color[index.y])
        + ray.v * (color[index.z]);
#endif
    }
  }

  if (flags & DG_TEXCOORD && self->texcoord) {
    //calculate texture coordinate using barycentric coordinates
    const uniform vec2f *uniform texcoord = self->texcoord;
    dg.st
      = (1.f-ray.u-ray.v) * (texcoord[index.x])
      + ray.u * (texcoord[index.y])
      + ray.v * (texcoord[index.z]);
  } else {
    dg.st = make_vec2f(0.0f, 0.0f);
  }

  if (flags & DG_TANGENTS) {
    uniform bool fallback = true;
    if (self->texcoord) {
      const uniform vec2f *uniform texcoord = self->texcoord;
      const vec2f dst02 = texcoord[index.x] - texcoord[index.z];
      const vec2f dst12 = texcoord[index.y] - texcoord[index.z];
      const float det = dst02.x * dst12.y - dst02.y * dst12.x;

      if (det != 0.f) {
        const float invDet = rcp(det);
        const uniform float *uniform vertex = self->vertex;
        const uniform int32 vtxSize = self->vtxSize;
        const vec3f *const varying v0ptr = (const uniform vec3f *)&vertex[index.x * vtxSize];
        const vec3f *const varying v1ptr = (const uniform vec3f *)&vertex[index.y * vtxSize];
        const vec3f *const varying v2ptr = (const uniform vec3f *)&vertex[index.z * vtxSize];
        const vec3f dp02 = *v0ptr - *v2ptr;
        const vec3f dp12 = *v1ptr - *v2ptr;
        dg.dPds = (dst12.y * dp02 - dst02.y * dp12) * invDet;
        dg.dPdt = (dst02.x * dp12 - dst12.x * dp02) * invDet;
        fallback = false;
      }
    }
    if (fallback) {
      linear3f f = frame(dg.Ng);
      dg.dPds = f.vx;
      dg.dPdt = f.vy;
    }
  }

  if (flags & DG_MATERIALID) {
    if (self->prim_materialID) {
      dg.materialID = self->prim_materialID[ray.primID];
    }
    // TODO: gather material ID from index array if indexSize==4
    else {
      dg.materialID = self->geom_materialID;
    }

    if( self->materialList) {
      Material *myMat = self->materialList[dg.materialID < 0 ? 0 : dg.materialID];
      dg.material = myMat;
    }
  }
}


//! constructor for ispc-side TriangleMesh object
void TriangleMesh_Constructor(uniform TriangleMesh *uniform mesh,
                              void *uniform cppEquivalent,
                              uniform Model *uniform model,
                              uniform int32  geomID,
                              uniform int32  numTriangles,
                              uniform int32  idxSize,
                              uniform int32  vtxSize,
                              uniform int32  norSize,
                              uniform int    *uniform index,
                              uniform float  *uniform vertex,
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
  mesh->vertex       = vertex;
  mesh->normal       = normal;
  mesh->color        = color;
  mesh->texcoord     = texcoord;
  mesh->idxSize      = idxSize;
  mesh->vtxSize      = vtxSize;
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

export void *uniform TriangleMesh_create(void *uniform cppEquivalent)
{
  uniform TriangleMesh *uniform mesh = uniform new uniform TriangleMesh;
  TriangleMesh_Constructor(mesh, cppEquivalent,
                           NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, -1, NULL, NULL, NULL);
  return mesh;
}

export void *uniform TriangleMesh_set(void *uniform _mesh,
                                      void *uniform _model,
                                      uniform int32  geomID,
                                      uniform int32  numTriangles,
                                      uniform int32  idxSize,
                                      uniform int32  vtxSize,
                                      uniform int32  norSize,
                                      uniform int    *uniform index,
                                      uniform float  *uniform vertex,
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
                           idxSize, vtxSize, norSize,
                           index,
                           vertex,
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
