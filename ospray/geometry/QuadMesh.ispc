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

int32 QuadMesh_getMaterialID(
    const Geometry *uniform const _self
    , const int32 primID
    )
{
  const QuadMesh *const uniform self = (const QuadMesh *uniform)_self;
  return self->prim_materialID ? self->prim_materialID[primID] : self->geom_materialID;
}

static void QuadMesh_postIntersect(uniform Geometry *uniform _self,
                                   uniform Model    *uniform model,
                                   varying DifferentialGeometry &dg,
                                   const varying Ray &ray,
                                   uniform int64 flags)
{
  QuadMesh *uniform self = (QuadMesh *uniform)_self;
  dg.Ng = dg.Ns = ray.Ng;
  const uniform bool huge_mesh = self->huge_mesh;
  const vec4i index = gather_vec4i(huge_mesh, self->index, ray.primID);
  const float u = ray.u;
  const float v = ray.v;
  vec4f uv;
#if 1 // bilinear for quads
  if (index.z == index.w)
    uv = make_vec4f(1.f-u-v, u, v, 0.f);
  else
    uv = make_vec4f((1-v)*(1-u), (1-v)*u, v*u, v*(1-u));
#else // always as triangles
  if (u+v < 1.0f)
    uv = make_vec4f(1.f-u-v, u, 0.f, v);
  else
    uv = make_vec4f(0.f, 1.f-v, u+v-1.f, 1.f-u);
#endif

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
  }

  if (flags & DG_TANGENTS) {
    uniform bool fallback = true;
    if (self->texcoord) {
      const uniform vec2f *uniform texcoord = self->texcoord;
      const vec2f a = gather_vec2f(huge_mesh, texcoord, index.x);
      const vec2f b = gather_vec2f(huge_mesh, texcoord, index.y);
      const vec2f c = gather_vec2f(huge_mesh, texcoord, index.w);
      const vec2f dst02 = a - c;
      const vec2f dst12 = b - c;
      const float det = dst02.x * dst12.y - dst02.y * dst12.x;

      if (det != 0.f) {
        const float invDet = rcp(det);
        const uniform float *uniform vertex = self->vertex;
        const uniform int32 vtxSize = self->vtxSize;
        const vec3f a = gather_vec3f(huge_mesh, vertex, vtxSize, index.x);
        const vec3f b = gather_vec3f(huge_mesh, vertex, vtxSize, index.y);
        const vec3f c = gather_vec3f(huge_mesh, vertex, vtxSize, index.w);
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

  if (flags & DG_MATERIALID)
    dg.materialID = QuadMesh_getMaterialID(_self, ray.primID);
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

  // gather vertices
  const uniform bool huge_mesh = self->huge_mesh;
  const vec4i index = gather_vec4i(huge_mesh, self->index, primID);
  const uniform float *uniform vertex = self->vertex;
  const uniform int32 vtxSize = self->vtxSize;
  // triangles a b c and d c b
  const vec3f a = gather_vec3f(huge_mesh, vertex, vtxSize, index.x);
  const vec3f b = gather_vec3f(huge_mesh, vertex, vtxSize, index.y);
  const vec3f c = gather_vec3f(huge_mesh, vertex, vtxSize, index.w);
  const vec3f d = gather_vec3f(huge_mesh, vertex, vtxSize, index.z);

  const vec3f e1 = xfmVector(xfm, a-c);
  const vec3f e2 = xfmVector(xfm, b-c);
  const vec3f m1 = cross(e1, e2);

  res.normal = m1;
  vec3f v0 = a;
  vec3f v1 = b;
  vec3f v2 = c;
  vec2f sp = s;

  if (index.z != index.w) {
    // painfully slow: re-calculate areas to decide which triangle to sample
    const vec3f e3 = xfmVector(xfm, d-b);
    const vec3f m2 = cross(e3, e2);
    const float a1 = length(m1);
    const float a2 = length(m2);
    const float p1 = a1*rcp(a1+a2);

    if (s.x < p1) {
      sp.x *= rcp(p1); // reproject
    } else { // sample second tri
      sp.x = (s.x - p1) * rcp(1.f - p1); // reproject
      v0 = d;
      v1 = c;
      v2 = b;
      res.normal = m2;
    }
  }

  const vec3f localPos = uniformSampleTriangle(v0, v1, v2, sp);
  res.normal = normalize(res.normal);
  res.pos = xfmPoint(xfm, localPos);
  return res;
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

  const float *const uniform vertex = self->vertex;
  const uniform int64 vtxSize = self->vtxSize;
  // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
  for (uniform int32 i = 0; i < numPrims; i++) {
    vec4i uniform index = self->index[primIDs[i]];
    // triangles a b c and d c b
    const uniform vec3f a = *((vec3f*)(vertex + vtxSize * index.x));
    const uniform vec3f b = *((vec3f*)(vertex + vtxSize * index.y));
    const uniform vec3f c = *((vec3f*)(vertex + vtxSize * index.w));
    const uniform vec3f d = *((vec3f*)(vertex + vtxSize * index.z));
    const uniform vec3f e1 = xfmVector(xfm, a-c);
    const uniform vec3f e2 = xfmVector(xfm, b-c);
    const uniform vec3f e3 = xfmVector(xfm, d-b);
    areas[i] = 0.5f*(length(cross(e1, e2)) + length(cross(e2, e3)));
  }
}


//! constructor for ispc-side QuadMesh object
void QuadMesh_Constructor(uniform QuadMesh *uniform mesh,
                          void *uniform cppEquivalent,
                          uniform Model *uniform model,
                          RTCGeometry geom,
                          uniform int32 geomID,
                          uniform int32  numQuads,
                          uniform int32  vtxSize,
                          uniform int32  norSize,
                          uniform vec4i  *uniform index,
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
                       NULL, NULL, 0, 0, 0, 0, NULL, NULL, NULL, NULL,
                       NULL, -1, NULL, NULL, true, false);
  return mesh;
}

export void *uniform QuadMesh_set(void *uniform _mesh,
                                  void *uniform _model,
                                  RTCGeometry geom,
                                  uniform int32 geomID,
                                  uniform int32  numQuads,
                                  uniform int32  vtxSize,
                                  uniform int32  norSize,
                                  uniform vec4i  *uniform index,
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
                       vtxSize, norSize,
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
