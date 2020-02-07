// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "common/Data.ih"
#include "common/Intersect.ih"
#include "common/Ray.ih"
#include "common/World.ih"
#include "geometry/Geometry.ih"
#include "math/box.ih"
#include "math/sampling.ih"
#include "math/vec.ih"

struct Spheres
{
  Geometry super;
  Data1D vertex;
  Data1D radius;
  Data1D texcoord;
  float global_radius;
};

static void Spheres_postIntersect(const Geometry *uniform geometry,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  Spheres *uniform self = (Spheres * uniform) geometry;

  dg.Ng = dg.Ns = ray.Ng;

  float radius = self->global_radius;
  if (valid(self->radius))
    radius = get_float(self->radius, ray.primID);

  // make epsilon large enough to not get lost when computing
  // |CO| = |center-ray.org| ~ radius for 2ndary rays
  dg.epsilon = radius * ulpEpsilon;

  if (and(flags & DG_TEXCOORD, valid(self->texcoord)))
    dg.st = get_vec2f(self->texcoord, ray.primID);
}

unmasked void Spheres_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  Spheres *uniform self = (Spheres * uniform) args->geometryUserPtr;
  uniform int primID = args->primID;
  uniform float radius = valid(self->radius) ? get_float(self->radius, primID)
                                             : self->global_radius;
  uniform vec3f center = get_vec3f(self->vertex, primID);
  box3fa *uniform out = (box3fa * uniform) args->bounds_o;
  *out = make_box3fa(center - radius, center + radius);
}

void Spheres_intersect_kernel(
    const RTCIntersectFunctionNArguments *uniform args,
    const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex])
    return;

  Spheres *uniform self = (Spheres * uniform) args->geometryUserPtr;
  uniform int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer to a varying ray!
  varying Ray *uniform ray = (varying Ray * uniform) args->rayhit;

  uniform float radius = valid(self->radius) ? get_float(self->radius, primID)
                                             : self->global_radius;
  uniform vec3f center = get_vec3f(self->vertex, primID);

  const Intersections isect =
      intersectSphere(ray->org, ray->dir, center, radius);

  bool hit = false;
  if (and(and(isect.entry.t > ray->t0, isect.entry.t <= ray->t),
          isect.entry.hit)) {
    hit = true;
    ray->t = isect.entry.t;
    ray->Ng = isect.entry.N;
  } else if (and(and(isect.exit.t > ray->t0, isect.exit.t <= ray->t),
                 isect.exit.hit)) {
    hit = true;
    ray->t = isect.exit.t;
    ray->Ng = isect.exit.N;
  }

  if (hit) {
    if (isOcclusionTest) {
      ray->t = neg_inf;
    } else {
      ray->primID = primID;
      ray->geomID = args->geomID;
      ray->instID = args->context->instID[0];
    }
  }
}

unmasked void Spheres_intersect(
    const struct RTCIntersectFunctionNArguments *uniform args)
{
  Spheres_intersect_kernel(args, false);
}

unmasked void Spheres_occluded(
    const struct RTCOccludedFunctionNArguments *uniform args)
{
  Spheres_intersect_kernel((RTCIntersectFunctionNArguments *)args, true);
}

SampleAreaRes Spheres_sampleArea(const Geometry *uniform const _self,
    const int32 primID,
    const uniform affine3f &xfm,
    const uniform affine3f &rcp_xfm,
    const vec2f &s)
{
  const Spheres *const uniform self = (const Spheres *uniform)_self;

  float radius = self->global_radius;
  if (valid(self->radius))
    radius = get_float(self->radius, primID);

  const vec3f localNormal = uniformSampleSphere(radius, s);
  const vec3f localPos = get_vec3f(self->vertex, primID) + localNormal;

  SampleAreaRes res;
  res.pos = xfmPoint(xfm, localPos);
  res.normal = normalize(xfmVector(transposed(rcp_xfm.l), localNormal));

  return res;
}

void Spheres_getAreas(const Geometry *const uniform _self,
    const int32 *const uniform primIDs,
    const uniform int32 numPrims,
    const uniform affine3f &xfm,
    float *const uniform areas)
{
  Spheres *uniform self = (Spheres * uniform) _self;

  // detect (non-uniform) scaling; get length of transformed unit-vectors
  const uniform vec3f scaling3 =
      make_vec3f(length(xfm.l.vx), length(xfm.l.vy), length(xfm.l.vz));
  const uniform float min_scaling = reduce_min(scaling3);
  const uniform float max_scaling = reduce_max(scaling3);
  if ((max_scaling - min_scaling) > 1e-4f * min_scaling) {
    print(
        "#osp:Spheres_getAreas: Non-uniform scaling in instance "
        "transformation detected! Importance sampling for emissive "
        "materials and thus resulting image may be wrong.\n");
  }
  // use average as approximation to closest uniform scaling
  const uniform float scale = four_pi * sqr(reduce_add(scaling3) / 3.f);

  if (!valid(self->radius)) {
    const uniform float sphereArea = scale * sqr(self->global_radius);
    foreach (i = 0 ... numPrims)
      areas[i] = sphereArea;
  } else {
    // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
    for (uniform int32 i = 0; i < numPrims; i++)
      areas[i] = scale * sqr(get_float(self->radius, i));
  }
}

void Spheres_Constructor(Spheres *uniform self, void *uniform cppEquivalent)
{
  Geometry_Constructor(&self->super, cppEquivalent, Spheres_postIntersect);
  self->super.getAreas = Spheres_getAreas;
  self->super.sampleArea = Spheres_sampleArea;
  Data1D_Constructor(&self->vertex);
  Data1D_Constructor(&self->radius);
  Data1D_Constructor(&self->texcoord);
  self->global_radius = 0.01;
}

export void *uniform Spheres_create(void *uniform cppEquivalent)
{
  Spheres *uniform self = uniform new Spheres;
  Spheres_Constructor(self, cppEquivalent);
  return self;
}

export void SpheresGeometry_set(void *uniform _self,
    void *uniform _embreeGeometry,
    const Data1D *uniform vertex,
    const Data1D *uniform radius,
    const Data1D *uniform texcoord,
    uniform float global_radius)
{
  Spheres *uniform self = (Spheres * uniform) _self;

  self->vertex = *vertex;
  self->radius = *radius;
  self->texcoord = *texcoord;
  self->global_radius = global_radius;
  self->super.numPrimitives = vertex->numItems;

  Geometry_setEmbreeUserGeometry(&self->super,
      (RTCGeometry)_embreeGeometry,
      Spheres_bounds,
      Spheres_intersect,
      Spheres_occluded);
}
