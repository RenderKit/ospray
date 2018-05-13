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
#include "QuadMesh.ih"
#include "common/Model.ih"
#include "common/safe_gather.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"

// embree
#include "embree3/rtcore.isph"

// Create quad_interpolate for vec2f, vec3f, and vec4f types
#define __define_quad_interpolate(T)              \
static inline T quad_interpolate(const vec4f& uv, \
                                 const T& t0,     \
                                 const T& t1,     \
                                 const T& t2,     \
                                 const T& t3) {   \
  return uv.x*t0 + uv.y*t1 + uv.z*t2 + uv.w*t3;   \
}

__define_quad_interpolate(vec2f);
__define_quad_interpolate(vec3f);
__define_quad_interpolate(vec4f);

static void QuadMesh_postIntersect(uniform Geometry *uniform _self,
                                   uniform Model    *uniform model,
                                   varying DifferentialGeometry &dg,
                                   const varying Ray &ray,
                                   uniform int64 flags)
{
  QuadMesh *uniform self = (QuadMesh *uniform)_self;
  dg.Ng = dg.Ns = ray.Ng;
  const uniform bool huge_mesh = self->huge_mesh;
  const vec4i index = gather_vec4i(huge_mesh, self->index, self->idxSize, ray.primID);
  const float u = ray.u;
  const float v = ray.v;
  const vec4f uv = make_vec4f((1-v)*(1-u), (1-v)*u, v*u, v*(1-u));

  if (flags & DG_NS && self->normal) {
    const uniform float *uniform normal = self->normal;
    const uniform int32 norSize = self->norSize;
    const vec3f a = gather_vec3f(huge_mesh, normal, norSize, index.x);
    const vec3f b = gather_vec3f(huge_mesh, normal, norSize, index.y);
    const vec3f c = gather_vec3f(huge_mesh, normal, norSize, index.z);
    const vec3f d = gather_vec3f(huge_mesh, normal, norSize, index.w);
    dg.Ns = quad_interpolate(uv, a, b, c, d);
  }

  if (flags & DG_COLOR && self->color) {
    const uniform vec4f *uniform color = self->color;
    const vec4f a = gather_vec4f(huge_mesh, color, index.x);
    const vec4f b = gather_vec4f(huge_mesh, color, index.y);
    const vec4f c = gather_vec4f(huge_mesh, color, index.z);
    const vec4f d = gather_vec4f(huge_mesh, color, index.w);
    dg.color = quad_interpolate(uv, a, b, c, d);
    if (!self->has_alpha)
      dg.color.w = 1.f;
  }

  if (flags & DG_TEXCOORD && self->texcoord) {
    const uniform vec2f *uniform texcoord = self->texcoord;
    const vec2f a = gather_vec2f(huge_mesh, texcoord, index.x);
    const vec2f b = gather_vec2f(huge_mesh, texcoord, index.y);
    const vec2f c = gather_vec2f(huge_mesh, texcoord, index.z);
    const vec2f d = gather_vec2f(huge_mesh, texcoord, index.w);
    dg.st = quad_interpolate(uv, a, b, c, d);
  } else
    dg.st = make_vec2f(0.0f, 0.0f);

  //TODO, fix for quads
  if (flags & DG_TANGENTS) {
    uniform bool fallback = true;
    if (self->texcoord) {
      const uniform vec2f *uniform texcoord = self->texcoord;
      const vec2f a = gather_vec2f(huge_mesh, texcoord, index.x);
      const vec2f b = gather_vec2f(huge_mesh, texcoord, index.y);
      const vec2f c = gather_vec2f(huge_mesh, texcoord, index.z);
      const vec2f dst02 = a - c;
      const vec2f dst12 = b - c;
      const float det = dst02.x * dst12.y - dst02.y * dst12.x;

      if (det != 0.f) {
        const float invDet = rcp(det);
        const uniform float *uniform vertex = self->vertex;
        const uniform int32 vtxSize = self->vtxSize;
        const vec3f a = gather_vec3f(huge_mesh, vertex, vtxSize, index.x);
        const vec3f b = gather_vec3f(huge_mesh, vertex, vtxSize, index.y);
        const vec3f c = gather_vec3f(huge_mesh, vertex, vtxSize, index.z);
        const vec3f dp02 = a - c;
        const vec3f dp12 = b - c;
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
  }
}

SampleAreaRes QuadMesh_sampleArea(
    const Geometry *uniform const _self
    , const int32 primID
    , const uniform affine3f &xfm
    , const uniform affine3f &
    , const vec2f& s
    )
{
  const QuadMesh *const uniform self = (const QuadMesh *uniform)_self;
  SampleAreaRes res;

  //TODO, fix for quads
  // gather vertices
  const uniform bool huge_mesh = self->huge_mesh;
  const vec3i index = gather_vec3i(huge_mesh, self->index, self->idxSize, primID);
  const uniform float *uniform vertex = self->vertex;
  const uniform int32 vtxSize = self->vtxSize;
  const vec3f a = gather_vec3f(huge_mesh, vertex, vtxSize, index.x);
  const vec3f b = gather_vec3f(huge_mesh, vertex, vtxSize, index.y);
  const vec3f c = gather_vec3f(huge_mesh, vertex, vtxSize, index.z);

  const vec3f localPos = uniformSampleTriangle(a, b, c, s);
  res.pos = xfmPoint(xfm, localPos);

  const vec3f e1 = a-c;
  const vec3f e2 = b-c;
  res.normal = normalize(cross(xfmVector(xfm, e1), xfmVector(xfm, e2)));

  return res;
}

int32 QuadMesh_getMaterialID(
    const Geometry *uniform const _self
    , const int32 primID
    )
{
  const QuadMesh *const uniform self = (const QuadMesh *uniform)_self;
  return self->prim_materialID ? self->prim_materialID[primID] : self->geom_materialID;
}

void QuadMesh_getAreas(
    const Geometry *const uniform _self
    , const int32 *const uniform primIDs
    , const uniform int32 numPrims
    , const uniform affine3f &xfm
    , float *const uniform areas
    )
{
  const QuadMesh *const uniform self = (const QuadMesh *uniform)_self;

  const uniform int32 idxSize = self->idxSize;
  const float *const uniform vertex = self->vertex;
  const uniform int64 vtxSize = self->vtxSize;
  // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
  for (uniform int32 i = 0; i < numPrims; i++) {
    int *uniform index = self->index + primIDs[i] * idxSize;
    const uniform vec3f a = *((vec3f*)(vertex + vtxSize * *index));
    const uniform vec3f b = *((vec3f*)(vertex + vtxSize * *(index+1)));
    const uniform vec3f c = *((vec3f*)(vertex + vtxSize * *(index+2)));
    const uniform vec3f e1 = xfmVector(xfm, a-c);
    const uniform vec3f e2 = xfmVector(xfm, b-c);
    areas[i] = 0.5f * length(cross(e1, e2));
  }
}


//! constructor for ispc-side QuadMesh object
void QuadMesh_Constructor(uniform QuadMesh *uniform mesh,
                          void *uniform cppEquivalent,
                          uniform Model *uniform model,
                          RTCGeometry geom,
                          uniform int32 geomID,
                          uniform int32  numQuads,
                          uniform int32  idxSize,
                          uniform int32  vtxSize,
                          uniform int32  norSize,
                          uniform int    *uniform index,
                          uniform float  *uniform vertex,
                          uniform float  *uniform normal,
                          uniform vec4f  *uniform color,
                          uniform vec2f  *uniform texcoord,
                          uniform int32   geom_materialID,
                          uniform Material *uniform *uniform materialList,
                          uniform uint32 *uniform prim_materialID,
                          uniform bool has_alpha,
                          uniform bool huge_mesh)
{
  Geometry_Constructor(&mesh->super,cppEquivalent,
                       QuadMesh_postIntersect,
                       model,geom, geomID,
                       materialList);
  mesh->super.getMaterialID = QuadMesh_getMaterialID;
  mesh->super.getAreas = QuadMesh_getAreas;
  mesh->super.sampleArea = QuadMesh_sampleArea;
  mesh->super.numPrimitives = numQuads;
  mesh->index        = index;
  mesh->vertex       = vertex;
  mesh->normal       = normal;
  mesh->color        = color;
  mesh->texcoord     = texcoord;
  mesh->idxSize      = idxSize;
  mesh->vtxSize      = vtxSize;
  mesh->norSize      = norSize;
  mesh->prim_materialID = prim_materialID;
  mesh->geom_materialID = geom_materialID;
  mesh->has_alpha = has_alpha;
  mesh->huge_mesh = huge_mesh;
}

export void *uniform QuadMesh_create(void *uniform cppEquivalent)
{
  QuadMesh *uniform mesh = uniform new QuadMesh;
  QuadMesh_Constructor(mesh, cppEquivalent,
                       NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL,
                       NULL, -1, NULL, NULL, true, false);
  return mesh;
}

export void *uniform QuadMesh_set(void *uniform _mesh,
                                  void *uniform _model,
                                  RTCGeometry geom,
                                  uniform int32 geomID,
                                  uniform int32  numQuads,
                                  uniform int32  idxSize,
                                  uniform int32  vtxSize,
                                  uniform int32  norSize,
                                  uniform int    *uniform index,
                                  uniform float  *uniform vertex,
                                  uniform float  *uniform normal,
                                  uniform vec4f  *uniform color,
                                  uniform vec2f  *uniform texcoord,
                                  uniform int32   geom_materialID,
                                  void *uniform _materialList,
                                  uniform uint32 *uniform prim_materialID,
                                  uniform bool has_alpha,
                                  uniform bool huge_mesh)
{
  uniform QuadMesh *uniform mesh = (uniform QuadMesh *uniform)_mesh;
  uniform Model *uniform model = (uniform Model *uniform)_model;
  uniform Material *uniform *uniform materialList = (uniform Material *uniform *uniform)_materialList;
  QuadMesh_Constructor(mesh,
                       mesh->super.cppEquivalent,
                       model,
                       geom,
                       geomID,
                       numQuads,
                       idxSize, vtxSize, norSize,
                       index,
                       vertex,
                       normal,
                       color,
                       texcoord,
                       geom_materialID,
                       (Material*uniform*uniform)materialList,
                       prim_materialID,
                       has_alpha,
                       huge_mesh);
}
