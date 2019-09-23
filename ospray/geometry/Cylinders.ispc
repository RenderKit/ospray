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
#include "common/Data.ih"
#include "common/Intersect.ih"
#include "common/Ray.ih"
#include "common/World.ih"
#include "geometry/Geometry.ih"
#include "math/box.ih"
#include "math/vec.ih"

struct Cylinders
{
  Geometry super;
  Data1D vertex0;
  Data1D vertex1;
  Data1D radius;
  Data1D texcoord0;
  Data1D texcoord1;
  float global_radius;
};

static void Cylinders_postIntersect(const Geometry *uniform _self,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  Cylinders *uniform self = (Cylinders * uniform) _self;
  dg.Ng = dg.Ns = ray.Ng;

  float radius = self->global_radius;
  cif (valid(self->radius))
    radius = get_float(self->radius, ray.primID);

  dg.epsilon = radius * ulpEpsilon;

  cif (and(flags & DG_TEXCOORD,
      and(valid(self->texcoord0), valid(self->texcoord1)))) {
    const vec2f tc0 = get_vec2f(self->texcoord0, ray.primID);
    const vec2f tc1 = get_vec2f(self->texcoord1, ray.primID);
    dg.st = lerp(ray.u, tc0, tc1);
  }
}

unmasked void Cylinders_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  Cylinders *uniform self = (Cylinders * uniform) args->geometryUserPtr;
  uniform int primID = args->primID;
  uniform float radius = valid(self->radius) ? get_float(self->radius, primID)
                                             : self->global_radius;
  uniform vec3f v0 = get_vec3f(self->vertex0, primID);
  uniform vec3f v1 = get_vec3f(self->vertex1, primID);

  uniform vec3f a2 = sqr(v0 - v1);
  uniform float d2 = a2.x + a2.y + a2.z;
  uniform vec3f a  = make_vec3f(a2.y + a2.z, a2.x + a2.z, a2.x + a2.y);
  uniform vec3f dr = radius * sqrt(a / d2);

  box3fa *uniform out = (box3fa * uniform) args->bounds_o;
  *out                = make_box3fa(min(v0, v1) - dr, max(v0, v1) + dr);
}

void Cylinders_intersect_kernel(
    const RTCIntersectFunctionNArguments *uniform args,
    const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex])
    return;

  Cylinders *uniform self = (Cylinders * uniform) args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer toa varying ray!
  varying Ray *uniform ray = (varying Ray * uniform) args->rayhit;

  uniform float radius = valid(self->radius) ? get_float(self->radius, primID)
                                             : self->global_radius;
  uniform vec3f v0 = get_vec3f(self->vertex0, primID);
  uniform vec3f v1 = get_vec3f(self->vertex1, primID);

  const Intersections isect =
      intersectCylinder(ray->org, ray->dir, v0, v1, radius);

  bool hit = false;
  if (and(and(isect.entry.t > ray->t0, isect.entry.t <= ray->t),
          isect.entry.hit)) {
    hit = true;
    ray->t = isect.entry.t;
    ray->u = isect.entry.u;
    ray->Ng = isect.entry.N;
  } else if (and(and(isect.exit.t > ray->t0, isect.exit.t <= ray->t),
                 isect.exit.hit)) {
    hit = true;
    ray->t = isect.exit.t;
    ray->u = isect.exit.u;
    ray->Ng = isect.exit.N;
  }

  if (hit) {
    if (isOcclusionTest) {
      ray->t = neg_inf;
    } else {
      ray->instID = args->context->instID[0];
      ray->primID = primID;
      ray->geomID = self->super.geomID;
    }
  }
}

unmasked void Cylinders_intersect(
    const struct RTCIntersectFunctionNArguments *uniform args)
{
  Cylinders_intersect_kernel(args, false);
}

unmasked void Cylinders_occluded(
    const struct RTCOccludedFunctionNArguments *uniform args)
{
  Cylinders_intersect_kernel(
      (RTCIntersectFunctionNArguments * uniform) args, true);
}

SampleAreaRes Cylinders_sampleArea(const Geometry *uniform const _self,
                                   const int32 primID,
                                   const uniform affine3f &xfm,
                                   const uniform affine3f &rcp_xfm,
                                   const vec2f &s)
{
  const Cylinders *const uniform self = (const Cylinders *uniform)_self;

  float radius = self->global_radius;
  cif (valid(self->radius))
    radius = get_float(self->radius, primID);

  vec3f v0 = get_vec3f(self->vertex0, primID);
  vec3f v1 = get_vec3f(self->vertex1, primID);

  linear3f frame  = frame(v1 - v0);
  frame.vy        = normalize(frame.vy);
  const float phi = two_pi * s.x;
  float sinPhi, cosPhi;
  sincos(phi, &sinPhi, &cosPhi);
  const vec3f localNormal =
      (cosPhi * radius) * frame.vx + (sinPhi * radius) * frame.vy;
  const vec3f localPos = v0 + localNormal + s.y * frame.vz;

  SampleAreaRes res;
  res.pos    = xfmPoint(xfm, localPos);
  res.normal = normalize(xfmVector(transposed(rcp_xfm.l), localNormal));

  return res;
}

void Cylinders_getAreas(const Geometry *const uniform _self,
                        const int32 *const uniform primIDs,
                        const uniform int32 numPrims,
                        const uniform affine3f &xfm,
                        float *const uniform areas)
{
  Cylinders *uniform self = (Cylinders * uniform) _self;

  // detect (non-uniform) scaling; get length of transformed unit-vectors
  const uniform vec3f scaling3 =
      make_vec3f(length(xfm.l.vx), length(xfm.l.vy), length(xfm.l.vz));
  const uniform float min_scaling = reduce_min(scaling3);
  const uniform float max_scaling = reduce_max(scaling3);
  if ((max_scaling - min_scaling) > 1e-4f * min_scaling)
    print(
        "#osp:Cylinders_getAreas: Non-uniform scaling in instance "
        "transformation detected! Importance sampling for emissive "
        "materials and thus resulting image may be wrong.\n");
  // use average as approximation to closest uniform scaling
  const uniform float scale = two_pi * reduce_add(scaling3) / 3.f;

  uniform float radius = self->global_radius;

  // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
  for (uniform int32 i = 0; i < numPrims; i++) {
    cif (valid(self->radius))
      radius = get_float(self->radius, i);

    const uniform vec3f v0 = get_vec3f(self->vertex0, i);
    const uniform vec3f v1 = get_vec3f(self->vertex1, i);
    areas[i] = scale * radius * length(v0 - v1);
  }
}

void Cylinders_Constructor(Cylinders *uniform self, void *uniform cppEquivalent)
{
  Geometry_Constructor(&self->super, cppEquivalent, Cylinders_postIntersect);
  self->super.getAreas = Cylinders_getAreas;
  self->super.sampleArea = Cylinders_sampleArea;
  Data1D_Constructor(&self->vertex0);
  Data1D_Constructor(&self->vertex1);
  Data1D_Constructor(&self->radius);
  Data1D_Constructor(&self->texcoord0);
  Data1D_Constructor(&self->texcoord1);
  self->global_radius = 0.01;
}

export void *uniform Cylinders_create(void *uniform cppEquivalent)
{
  Cylinders *uniform self = uniform new Cylinders;
  Cylinders_Constructor(self, cppEquivalent);
  return self;
}

export void CylindersGeometry_set(void *uniform _self,
    void *uniform _embreeGeometry,
    const Data1D *uniform vertex0,
    const Data1D *uniform vertex1,
    const Data1D *uniform radius,
    const Data1D *uniform texcoord0,
    const Data1D *uniform texcoord1,
    uniform float global_radius)
{
  Cylinders *uniform self = (Cylinders * uniform) _self;

  self->vertex0 = *vertex0;
  self->vertex1 = *vertex1;
  self->radius = *radius;
  self->texcoord0 = *texcoord0;
  self->texcoord1 = *texcoord1;
  self->global_radius = global_radius;
  self->super.numPrimitives = vertex0->numItems;

  Geometry_setEmbreeUserGeometry(&self->super,
      (RTCGeometry)_embreeGeometry,
      Cylinders_bounds,
      Cylinders_intersect,
      Cylinders_occluded);
}
