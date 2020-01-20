// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Geometry.ih"
#include "common/Data.ih"
#include "common/World.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"

struct Mesh
{
  Geometry super;
  Data1D index;
  Data1D vertex;
  Data1D normal;
  Data1D color;
  Data1D texcoord;
  int64 flagMask; // which attributes are missing and cannot be interpolated
  bool has_alpha; // 4th color component is valid
  bool is_triangleMesh;
};

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

static void QuadMesh_postIntersect(const Geometry *uniform _self,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  Mesh *uniform self = (Mesh * uniform) _self;
  dg.Ng = dg.Ns = ray.Ng;
  const vec4ui index = get_vec4ui(self->index, ray.primID);
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
    const vec3f a = get_vec3f(self->normal, index.x);
    const vec3f b = get_vec3f(self->normal, index.y);
    const vec3f c = get_vec3f(self->normal, index.z);
    const vec3f d = get_vec3f(self->normal, index.w);
    dg.Ns = quad_interpolate(uv, a, b, c, d);
  }

  if (flags & DG_COLOR) {
    const vec4f a = get_vec4f(self->color, index.x);
    const vec4f b = get_vec4f(self->color, index.y);
    const vec4f c = get_vec4f(self->color, index.z);
    const vec4f d = get_vec4f(self->color, index.w);
    dg.color = quad_interpolate(uv, a, b, c, d);
    if (!self->has_alpha)
      dg.color.w = 1.f;
  }

  const uniform bool compute_texcoord = flags & DG_TEXCOORD;
  bool compute_tangents = flags & DG_TANGENTS;
  if (and(or (compute_texcoord, compute_tangents), valid(self->texcoord))) {
    const vec2f a = get_vec2f(self->texcoord, index.x);
    const vec2f b = get_vec2f(self->texcoord, index.y);
    const vec2f d = get_vec2f(self->texcoord, index.w);
    if (compute_texcoord) {
      const vec2f c = get_vec2f(self->texcoord, index.z);
      dg.st = quad_interpolate(uv, a, b, c, d);
    }
    if (compute_tangents) {
      const vec2f dst02 = a - d;
      const vec2f dst12 = b - d;
      const float det = dst02.x * dst12.y - dst02.y * dst12.x;

      if (det != 0.f) {
        const float invDet = rcp(det);
        const vec3f a = get_vec3f(self->vertex, index.x);
        const vec3f b = get_vec3f(self->vertex, index.y);
        const vec3f d = get_vec3f(self->vertex, index.w);
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

static void TriangleMesh_PostIntersect(const Geometry *uniform _self,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  Mesh *uniform self = (Mesh * uniform) _self;
  dg.Ng = dg.Ns = ray.Ng;
  const vec3ui index = get_vec3ui(self->index, ray.primID);
  const vec3f uv = make_vec3f(1.0f - ray.u - ray.v, ray.u, ray.v);

  flags &= self->flagMask;

  if (flags & DG_NS) {
    const vec3f a = get_vec3f(self->normal, index.x);
    const vec3f b = get_vec3f(self->normal, index.y);
    const vec3f c = get_vec3f(self->normal, index.z);
    dg.Ns = interpolate(uv, a, b, c);
  }

  if (flags & DG_COLOR) {
    const vec4f a = get_vec4f(self->color, index.x);
    const vec4f b = get_vec4f(self->color, index.y);
    const vec4f c = get_vec4f(self->color, index.z);
    dg.color = interpolate(uv, a, b, c);
    if (!self->has_alpha)
      dg.color.w = 1.f;
  }

  const uniform bool compute_texcoord = flags & DG_TEXCOORD;
  bool compute_tangents = flags & DG_TANGENTS;
  if (and(or (compute_texcoord, compute_tangents), valid(self->texcoord))) {
    const vec2f a = get_vec2f(self->texcoord, index.x);
    const vec2f b = get_vec2f(self->texcoord, index.y);
    const vec2f c = get_vec2f(self->texcoord, index.z);
    if (compute_texcoord)
      dg.st = interpolate(uv, a, b, c);
    if (compute_tangents) {
      const vec2f dst02 = a - c;
      const vec2f dst12 = b - c;
      const float det = dst02.x * dst12.y - dst02.y * dst12.x;

      if (det != 0.f) {
        const float invDet = rcp(det);
        const vec3f a = get_vec3f(self->vertex, index.x);
        const vec3f b = get_vec3f(self->vertex, index.y);
        const vec3f c = get_vec3f(self->vertex, index.z);
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
    const vec2f &s)
{
  const Mesh *const uniform self = (const Mesh *uniform)_self;
  SampleAreaRes res;
  vec4ui index4;
  if (self->is_triangleMesh) {
    const vec3ui index3 = get_vec3ui(self->index, primID);
    index4 = make_vec4ui(index3.x, index3.y, index3.z, index3.z);
  } else
    index4 = get_vec4ui(self->index, primID);
  const vec4ui index = index4;

  const vec3f a = get_vec3f(self->vertex, index.x);
  const vec3f b = get_vec3f(self->vertex, index.y);
  const vec3f c = get_vec3f(self->vertex, index.z);
  const vec3f e1 = xfmVector(xfm, a - c);
  const vec3f e2 = xfmVector(xfm, b - c);
  const vec3f m1 = cross(e1, e2);

  res.normal = m1;
  vec3f v0 = a;
  vec3f v1 = b;
  vec3f v2 = c;
  vec2f sp = s;

  if (index.z != index.w) {
    const vec3f d = get_vec3f(self->vertex, index.w);
    // painfully slow: re-calculate areas to decide which triangle to sample
    const vec3f e3 = xfmVector(xfm, d - b);
    const vec3f m2 = cross(e3, e2);
    const float a1 = length(m1);
    const float a2 = length(m2);
    const float p1 = a1 * rcp(a1 + a2);

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

void Mesh_getAreas(const Geometry *const uniform _self,
    const int32 *const uniform primIDs,
    const uniform int32 numPrims,
    const uniform affine3f &xfm,
    float *const uniform areas)
{
  const Mesh *const uniform self = (const Mesh *uniform)_self;
  // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
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
      const uniform vec3f e3 = xfmVector(xfm, d - b);
      areas[i] += 0.5f * length(cross(e2, e3));
    }
  }
}

void Mesh_Constructor(Mesh *uniform self, void *uniform cppEquivalent)
{
  Geometry_Constructor(&self->super, cppEquivalent, NULL);
  self->super.getAreas = Mesh_getAreas;
  self->super.sampleArea = Mesh_sampleArea;
  Data1D_Constructor(&self->index);
  Data1D_Constructor(&self->vertex);
  Data1D_Constructor(&self->normal);
  Data1D_Constructor(&self->color);
  Data1D_Constructor(&self->texcoord);
}

export void *uniform Mesh_create(void *uniform cppEquivalent)
{
  Mesh *uniform self = uniform new Mesh;
  Mesh_Constructor(self, cppEquivalent);
  return self;
}

export void *uniform Mesh_set(void *uniform _self,
    const Data1D *uniform index,
    const Data1D *uniform vertex,
    const Data1D *uniform normal,
    const Data1D *uniform color,
    const Data1D *uniform texcoord,
    uniform bool has_alpha,
    uniform bool is_triangleMesh)
{
  Mesh *uniform self = (Mesh * uniform) _self;
  self->index = *index;
  self->vertex = *vertex;
  self->normal = *normal;
  self->color = *color;
  self->texcoord = *texcoord;
  self->has_alpha = has_alpha;
  self->super.numPrimitives = index->numItems;
  self->is_triangleMesh = is_triangleMesh;
  self->super.postIntersect = self->is_triangleMesh ? TriangleMesh_PostIntersect
                                                    : QuadMesh_postIntersect;

  self->flagMask = -1;
  if (!valid(self->normal))
    self->flagMask &= ~DG_NS;
  if (!valid(self->color))
    self->flagMask &= ~DG_COLOR;
  if (!valid(self->texcoord))
    self->flagMask &= ~DG_TEXCOORD;
}
