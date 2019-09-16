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
#include "Geometry.ih"
#include "common/Data.ih"
#include "common/World.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"

struct TriangleMesh
{
  Geometry super;
  Data1D index;
  Data1D vertex;
  Data1D normal;
  Data1D color;
  Data1D texcoord;
  int64 flagMask; // which attributes are missing and cannot be interpolated
  bool has_alpha;         // 4th color component is valid
};

static void TriangleMesh_postIntersect(const Geometry *uniform _self,
                                       varying DifferentialGeometry &dg,
                                       const varying Ray &ray,
                                       uniform int64 flags)
{
  const TriangleMesh *uniform self = (const TriangleMesh *uniform)_self;
  dg.Ng = dg.Ns                = ray.Ng;
  const vec3i index = get_vec3i(self->index, ray.primID);
  vec3f bary = make_vec3f(1.0f - ray.u - ray.v, ray.u, ray.v);

  flags &= self->flagMask;

  cif (flags & DG_NS) {
    const vec3f a = get_vec3f(self->normal, index.x);
    const vec3f b = get_vec3f(self->normal, index.y);
    const vec3f c = get_vec3f(self->normal, index.z);
    dg.Ns         = interpolate(bary, a, b, c);
  }

  cif (flags & DG_COLOR) {
    const vec4f a = get_vec4f(self->color, index.x);
    const vec4f b = get_vec4f(self->color, index.y);
    const vec4f c = get_vec4f(self->color, index.z);
    dg.color      = interpolate(bary, a, b, c);
    if (!self->has_alpha)
      dg.color.w = 1.f;
  }

  const uniform bool compute_texcoord = flags & DG_TEXCOORD;
  bool compute_tangents = flags & DG_TANGENTS;
  cif (and(or(compute_texcoord, compute_tangents), valid(self->texcoord))) {
    const vec2f a = get_vec2f(self->texcoord, index.x);
    const vec2f b = get_vec2f(self->texcoord, index.y);
    const vec2f c = get_vec2f(self->texcoord, index.z);
    cif (compute_texcoord)
      dg.st = interpolate(bary, a, b, c);
    cif (compute_tangents) {
      const vec2f dst02 = a - c;
      const vec2f dst12 = b - c;
      const float det   = dst02.x * dst12.y - dst02.y * dst12.x;

      if (det != 0.f) {
        const float invDet                  = rcp(det);
        const vec3f a = get_vec3f(self->vertex, index.x);
        const vec3f b = get_vec3f(self->vertex, index.y);
        const vec3f c = get_vec3f(self->vertex, index.z);
        const vec3f dp02 = a - c;
        const vec3f dp12 = b - c;
        dg.dPds          = (dst12.y * dp02 - dst02.y * dp12) * invDet;
        dg.dPdt          = (dst02.x * dp12 - dst12.x * dp02) * invDet;
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

SampleAreaRes TriangleMesh_sampleArea(const Geometry *uniform const _self,
                                      const int32 primID,
                                      const uniform affine3f &xfm,
                                      const uniform affine3f &,
                                      const vec2f &s)
{
  const TriangleMesh *const uniform self = (const TriangleMesh *uniform)_self;
  SampleAreaRes res;

  // gather vertices
  const vec3i index = get_vec3i(self->index, primID);
  const vec3f a = get_vec3f(self->vertex, index.x);
  const vec3f b = get_vec3f(self->vertex, index.y);
  const vec3f c = get_vec3f(self->vertex, index.z);

  const vec3f localPos = uniformSampleTriangle(a, b, c, s);
  res.pos              = xfmPoint(xfm, localPos);

  const vec3f e1 = a - c;
  const vec3f e2 = b - c;
  res.normal     = normalize(cross(xfmVector(xfm, e1), xfmVector(xfm, e2)));

  return res;
}

void TriangleMesh_getAreas(const Geometry *const uniform _self,
                           const int32 *const uniform primIDs,
                           const uniform int32 numPrims,
                           const uniform affine3f &xfm,
                           float *const uniform areas)
{
  const TriangleMesh *const uniform self = (const TriangleMesh *uniform)_self;

  // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
  for (uniform int32 i = 0; i < numPrims; i++) {
    const uniform vec3i index = get_vec3i(self->index, primIDs[i]);
    const uniform vec3f a = get_vec3f(self->vertex, index.x);
    const uniform vec3f b = get_vec3f(self->vertex, index.y);
    const uniform vec3f c = get_vec3f(self->vertex, index.z);
    const uniform vec3f e1 = xfmVector(xfm, a - c);
    const uniform vec3f e2 = xfmVector(xfm, b - c);
    areas[i]               = 0.5f * length(cross(e1, e2));
  }
}

void TriangleMesh_Constructor(
    TriangleMesh *uniform self, void *uniform cppEquivalent)
{
  Geometry_Constructor(&self->super, cppEquivalent, TriangleMesh_postIntersect);
  self->super.getAreas = TriangleMesh_getAreas;
  self->super.sampleArea = TriangleMesh_sampleArea;
  Data1D_Constructor(&self->index);
  Data1D_Constructor(&self->vertex);
  Data1D_Constructor(&self->normal);
  Data1D_Constructor(&self->color);
  Data1D_Constructor(&self->texcoord);
}

export void *uniform TriangleMesh_create(void *uniform cppEquivalent)
{
  TriangleMesh *uniform self = uniform new TriangleMesh;
  TriangleMesh_Constructor(self, cppEquivalent);
  return self;
}

export void *uniform TriangleMesh_set(void *uniform _self,
    const Data1D *uniform index,
    const Data1D *uniform vertex,
    const Data1D *uniform normal,
    const Data1D *uniform color,
    const Data1D *uniform texcoord,
    uniform bool has_alpha)
{
  TriangleMesh *uniform self = (TriangleMesh * uniform) _self;
  self->index = *index;
  self->vertex = *vertex;
  self->normal = *normal;
  self->color = *color;
  self->texcoord = *texcoord;
  self->has_alpha = has_alpha;
  self->super.numPrimitives = index->numItems;

  self->flagMask = -1;
  cif (!valid(self->normal))
    self->flagMask &= ~DG_NS;
  cif (!valid(self->color))
    self->flagMask &= ~DG_COLOR;
  cif (!valid(self->texcoord))
    self->flagMask &= ~DG_TEXCOORD;
}
