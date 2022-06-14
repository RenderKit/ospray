// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Geometry.ih"
#include "common/Data.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"
// c++ shared
#include "MeshShared.h"

// Create quad_interpolate for vec2f, vec3f, and vec4f types
#define __define_quad_interpolate(T)                                           \
  static inline T quad_interpolate(                                            \
      const vec4f &uv, const T &t0, const T &t1, const T &t2, const T &t3)     \
  {                                                                            \
    return uv.x * t0 + uv.y * t1 + uv.z * t2 + uv.w * t3;                      \
  }

__define_quad_interpolate(vec2f);
__define_quad_interpolate(vec3f);
__define_quad_interpolate(vec4f);

inline float Mesh_calcKey(Mesh *uniform self, float time, int &idx)
{
  // should only get hits within time interval, just to be sure...
  float t = clamp(time, self->time.lower, self->time.upper);
  t = (t - self->time.lower) * rcp(box_size(self->time))
      * (self->motionKeys - 1);
  idx = clamp((int)t, 0, self->motionKeys - 2);
  t = clamp(t - idx);
  return t;
}

inline vec3f interpolate_vec3f(const uniform int64 byteStride,
    uint8 *a0,
    uint8 *a1,
    const int index,
    const float t)
{
  vec3f v0, v1;
  int64 offs = byteStride * index;
  v0 = *((const vec3f *)(a0 + offs));
  v1 = *((const vec3f *)(a1 + offs));
  return lerp(t, v0, v1);
}

void QuadMesh_postIntersect(const Geometry *uniform _self,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  Mesh *uniform self = (Mesh * uniform) _self;
  dg.Ng = dg.Ns = ray.Ng;
  const vec4ui index = get_vec4ui(self->index, ray.primID);
  const iOffset = ray.primID * 4;
  const vec4ui findex = make_vec4ui(
      iOffset, iOffset + 1, iOffset + 2, iOffset + 3); // face indices
  const float u = ray.u;
  const float v = ray.v;
  vec4f uv;
#if 1 // bilinear for quads
  if (index.z == index.w)
    uv = make_vec4f(1.f - u - v, u, v, 0.f);
  else
    uv = make_vec4f((1 - v) * (1 - u), (1 - v) * u, v * u, v * (1 - u));
#else // always as triangles
  if (u + v < 1.0f)
    uv = make_vec4f(1.f - u - v, u, 0.f, v);
  else
    uv = make_vec4f(0.f, 1.f - v, u + v - 1.f, 1.f - u);
#endif

  flags &= self->flagMask;

  if (flags & DG_NS) {
    const vec4ui tindex = self->isNormalFaceVarying ? findex : index;
    if (self->motionKeys) {
      int idx;
      const float t = Mesh_calcKey(self, ray.time, idx);
      const uniform int64 byteStride = self->normal.byteStride;
      const uint8 *a0 = self->motionNormal[idx];
      const uint8 *a1 = self->motionNormal[idx + 1];
      const vec3f a = interpolate_vec3f(byteStride, a0, a1, tindex.x, t);
      const vec3f b = interpolate_vec3f(byteStride, a0, a1, tindex.y, t);
      const vec3f c = interpolate_vec3f(byteStride, a0, a1, tindex.z, t);
      const vec3f d = interpolate_vec3f(byteStride, a0, a1, tindex.w, t);
      dg.Ns = quad_interpolate(uv, a, b, c, d);
    } else {
      const vec3f a = get_vec3f(self->normal, tindex.x);
      const vec3f b = get_vec3f(self->normal, tindex.y);
      const vec3f c = get_vec3f(self->normal, tindex.z);
      const vec3f d = get_vec3f(self->normal, tindex.w);
      dg.Ns = quad_interpolate(uv, a, b, c, d);
    }
  }

  if (flags & DG_COLOR) {
    const vec4ui tindex = self->isColorFaceVarying ? findex : index;
    const vec4f a = get_vec4f(self->color, tindex.x);
    const vec4f b = get_vec4f(self->color, tindex.y);
    const vec4f c = get_vec4f(self->color, tindex.z);
    const vec4f d = get_vec4f(self->color, tindex.w);
    dg.color = quad_interpolate(uv, a, b, c, d);
    if (!self->has_alpha)
      dg.color.w = 1.f;
  }

  const uniform bool compute_texcoord = flags & DG_TEXCOORD;
  bool compute_tangents = flags & DG_TANGENTS;
  if (and(or (compute_texcoord, compute_tangents), valid(self->texcoord))) {
    const vec4ui tindex = self->isTexcoordFaceVarying ? findex : index;
    const vec2f a = get_vec2f(self->texcoord, tindex.x);
    const vec2f b = get_vec2f(self->texcoord, tindex.y);
    const vec2f d = get_vec2f(self->texcoord, tindex.w);
    if (compute_texcoord) {
      const vec2f c = get_vec2f(self->texcoord, tindex.z);
      dg.st = quad_interpolate(uv, a, b, c, d);
    }
    if (compute_tangents) {
      const vec2f dst02 = a - d;
      const vec2f dst12 = b - d;
      const float det = dst02.x * dst12.y - dst02.y * dst12.x;

      if (det != 0.f) {
        const float invDet = rcp(det);
        vec3f a, b, d;
        if (self->motionKeys) {
          int idx;
          const float t = Mesh_calcKey(self, ray.time, idx);
          const uniform int64 byteStride = self->vertex.byteStride;
          const uint8 *a0 = self->motionVertex[idx];
          const uint8 *a1 = self->motionVertex[idx + 1];
          a = interpolate_vec3f(byteStride, a0, a1, index.x, t);
          b = interpolate_vec3f(byteStride, a0, a1, index.y, t);
          d = interpolate_vec3f(byteStride, a0, a1, index.w, t);
        } else {
          a = get_vec3f(self->vertex, index.x);
          b = get_vec3f(self->vertex, index.y);
          d = get_vec3f(self->vertex, index.w);
        }
        const vec3f dp02 = a - d;
        const vec3f dp12 = b - d;
        dg.dPds = (dst12.y * dp02 - dst02.y * dp12) * invDet;
        dg.dPdt = (dst02.x * dp12 - dst12.x * dp02) * invDet;
        compute_tangents = false;
      }
    }
  }
  if (compute_tangents) {
    linear3f f = frame(dg.Ng);
    dg.dPds = f.vx;
    dg.dPdt = f.vy;
  }
}

void TriangleMesh_PostIntersect(const Geometry *uniform _self,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  Mesh *uniform self = (Mesh * uniform) _self;
  dg.Ng = dg.Ns = ray.Ng;
  const vec3ui index = get_vec3ui(self->index, ray.primID);
  const iOffset = ray.primID * 3;
  const vec3ui findex =
      make_vec3ui(iOffset, iOffset + 1, iOffset + 2); // face indices
  const vec3f uv = make_vec3f(1.0f - ray.u - ray.v, ray.u, ray.v);

  flags &= self->flagMask;

  if (flags & DG_NS) {
    const vec3ui tindex = self->isNormalFaceVarying ? findex : index;
    if (self->motionKeys) {
      int idx;
      const float t = Mesh_calcKey(self, ray.time, idx);
      const uniform int64 byteStride = self->normal.byteStride;
      const uint8 *a0 = self->motionNormal[idx];
      const uint8 *a1 = self->motionNormal[idx + 1];
      const vec3f a = interpolate_vec3f(byteStride, a0, a1, tindex.x, t);
      const vec3f b = interpolate_vec3f(byteStride, a0, a1, tindex.y, t);
      const vec3f c = interpolate_vec3f(byteStride, a0, a1, tindex.z, t);
      dg.Ns = interpolate(uv, a, b, c);
    } else {
      const vec3f a = get_vec3f(self->normal, tindex.x);
      const vec3f b = get_vec3f(self->normal, tindex.y);
      const vec3f c = get_vec3f(self->normal, tindex.z);
      dg.Ns = interpolate(uv, a, b, c);
    }
  }

  if (flags & DG_COLOR) {
    const vec3ui tindex = self->isColorFaceVarying ? findex : index;
    const vec4f a = get_vec4f(self->color, tindex.x);
    const vec4f b = get_vec4f(self->color, tindex.y);
    const vec4f c = get_vec4f(self->color, tindex.z);
    dg.color = interpolate(uv, a, b, c);
    if (!self->has_alpha)
      dg.color.w = 1.f;
  }

  const uniform bool compute_texcoord = flags & DG_TEXCOORD;
  bool compute_tangents = flags & DG_TANGENTS;
  if (and(or (compute_texcoord, compute_tangents), valid(self->texcoord))) {
    const vec3ui tindex = self->isTexcoordFaceVarying ? findex : index;
    const vec2f a = get_vec2f(self->texcoord, tindex.x);
    const vec2f b = get_vec2f(self->texcoord, tindex.y);
    const vec2f c = get_vec2f(self->texcoord, tindex.z);
    if (compute_texcoord)
      dg.st = interpolate(uv, a, b, c);
    if (compute_tangents) {
      const vec2f dst02 = a - c;
      const vec2f dst12 = b - c;
      const float det = dst02.x * dst12.y - dst02.y * dst12.x;

      if (det != 0.f) {
        const float invDet = rcp(det);
        vec3f a, b, c;
        if (self->motionKeys) {
          int idx;
          const float t = Mesh_calcKey(self, ray.time, idx);
          const uniform int64 byteStride = self->vertex.byteStride;
          const uint8 *a0 = self->motionVertex[idx];
          const uint8 *a1 = self->motionVertex[idx + 1];
          a = interpolate_vec3f(byteStride, a0, a1, index.x, t);
          b = interpolate_vec3f(byteStride, a0, a1, index.y, t);
          c = interpolate_vec3f(byteStride, a0, a1, index.z, t);
        } else {
          a = get_vec3f(self->vertex, index.x);
          b = get_vec3f(self->vertex, index.y);
          c = get_vec3f(self->vertex, index.z);
        }
        const vec3f dp02 = a - c;
        const vec3f dp12 = b - c;
        dg.dPds = (dst12.y * dp02 - dst02.y * dp12) * invDet;
        dg.dPdt = (dst02.x * dp12 - dst12.x * dp02) * invDet;
        compute_tangents = false;
      }
    }
  }
  if (compute_tangents) {
    linear3f f = frame(dg.Ng);
    dg.dPds = f.vx;
    dg.dPdt = f.vy;
  }
}

SampleAreaRes Mesh_sampleArea(const Geometry *uniform const _self,
    const int32 primID,
    const uniform affine3f &xfm,
    const uniform affine3f &,
    const vec2f &s,
    const float time)
{
  const Mesh *const uniform self = (const Mesh *uniform)_self;
  SampleAreaRes res;
  vec4ui index4;
  bool quad = false;
  if (self->is_triangleMesh) {
    const vec3ui index3 = get_vec3ui(self->index, primID);
    index4 = make_vec4ui(index3.x, index3.y, index3.z, index3.z);
  } else {
    index4 = get_vec4ui(self->index, primID);
    quad = index4.z != index4.w;
  }
  const vec4ui index = index4;

  vec3f a, b, c, d;
  if (self->motionKeys) {
    int idx;
    const float t = Mesh_calcKey(self, time, idx);
    const uniform int64 byteStride = self->vertex.byteStride;
    const uint8 *a0 = self->motionVertex[idx];
    const uint8 *a1 = self->motionVertex[idx + 1];
    a = interpolate_vec3f(byteStride, a0, a1, index.x, t);
    b = interpolate_vec3f(byteStride, a0, a1, index.y, t);
    c = interpolate_vec3f(byteStride, a0, a1, index.z, t);
    if (quad)
      d = interpolate_vec3f(byteStride, a0, a1, index.w, t);
  } else {
    a = get_vec3f(self->vertex, index.x);
    b = get_vec3f(self->vertex, index.y);
    c = get_vec3f(self->vertex, index.z);
    if (quad)
      d = get_vec3f(self->vertex, index.w);
  }
  const vec3f e1 = xfmVector(xfm, a - c);
  const vec3f e2 = xfmVector(xfm, b - c);
  const vec3f m1 = cross(e1, e2);

  res.normal = m1;
  vec3f v0 = a;
  vec3f v1 = b;
  vec3f v2 = c;
  vec2f sp = s;

  if (quad) {
    // painfully slow: re-calculate areas to decide which triangle to sample
    const vec3f e3 = xfmVector(xfm, d - c);
    const vec3f m2 = cross(e2, e3);
    const float a1 = length(m1);
    const float a2 = length(m2);
    const float p1 = a1 * rcp(a1 + a2);

    if (s.x < p1) {
      sp.x *= rcp(p1); // reproject
    } else { // sample second tri
      sp.x = (s.x - p1) * rcp(1.f - p1); // reproject
      v0 = d;
      v1 = c;
      v2 = a;
      res.normal = m2;
    }
  }

  const vec3f localPos = uniformSampleTriangle(v0, v1, v2, sp);
  res.normal = normalize(res.normal);
  res.pos = xfmPoint(xfm, localPos);
  return res;
}

void Mesh_getAreas(const Geometry *const uniform _self,
    const int32 *const uniform primIDs,
    const uniform int32 numPrims,
    const uniform affine3f &xfm,
    float *const uniform areas)
{
  const Mesh *const uniform self = (const Mesh *uniform)_self;
  // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
  // XXX for deformation motion blur the areas (i.e. selection probabilities)
  // are calculated using the first key only
  for (uniform int32 i = 0; i < numPrims; i++) {
    const uniform vec4ui index = self->is_triangleMesh
        ? make_vec4ui(get_vec3ui(self->index, primIDs[i]))
        : get_vec4ui(self->index, primIDs[i]);
    const uniform vec3f a = get_vec3f(self->vertex, index.x);
    const uniform vec3f b = get_vec3f(self->vertex, index.y);
    const uniform vec3f c = get_vec3f(self->vertex, index.z);
    const uniform vec3f e1 = xfmVector(xfm, a - c);
    const uniform vec3f e2 = xfmVector(xfm, b - c);
    areas[i] = 0.5f * length(cross(e1, e2));
    if (!self->is_triangleMesh) {
      const uniform vec3f d = get_vec3f(self->vertex, index.w);
      const uniform vec3f e3 = xfmVector(xfm, d - c);
      areas[i] += 0.5f * length(cross(e2, e3));
    }
  }
}

export void *uniform QuadMesh_postIntersect_addr()
{
  return QuadMesh_postIntersect;
}

export void *uniform TriangleMesh_PostIntersect_addr()
{
  return TriangleMesh_PostIntersect;
}

export void *uniform Mesh_sampleArea_addr()
{
  return Mesh_sampleArea;
}

export void *uniform Mesh_getAreas_addr()
{
  return Mesh_getAreas;
}
