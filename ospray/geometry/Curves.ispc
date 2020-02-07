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

struct Curves
{
  Geometry super; //!< inherited geometry fields
  Data1D index;
  Data1D vertex;
  Data1D color;
  Data1D texcoord;
  float radius;
  int64 flagMask;
  RTCGeometry geom;
};

static void Curves_postIntersect(const Geometry *uniform _self,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  Curves *uniform self = (Curves * uniform) _self;
  dg.Ng = dg.Ns = ray.Ng;
  flags &= self->flagMask;
  if (flags & DG_COLOR) {
    rtcInterpolateV1(self->geom,
        ray.primID,
        ray.u,
        ray.v,
        RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        0,
        &dg.color.x,
        NULL,
        NULL,
        4);
  }
  if (flags & DG_TEXCOORD) {
    rtcInterpolateV1(self->geom,
        ray.primID,
        ray.u,
        ray.v,
        RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        1,
        &dg.st.x,
        NULL,
        NULL,
        2);
  }
}

static void CurvesUserGeometry_postIntersect(const Geometry *uniform self,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  Curves *uniform self = (Curves * uniform) self;

  dg.Ng = dg.Ns = ray.Ng;
  dg.epsilon = self->radius * ulpEpsilon;

  if (and(flags & DG_COLOR, valid(self->color))) {
    const uint32 index = get_int32(self->index, ray.primID);
    const vec4f col0 = get_vec4f(self->color, index);
    const vec4f col1 = get_vec4f(self->color, index + 1);
    dg.color = lerp(ray.u, col0, col1);
  }

  if (and(flags & DG_TEXCOORD, valid(self->texcoord))) {
    const uint32 index = get_int32(self->index, ray.primID);
    const vec2f tc0 = get_vec2f(self->texcoord, index);
    const vec2f tc1 = get_vec2f(self->texcoord, index + 1);
    dg.st = lerp(ray.u, tc0, tc1);
  }
}

unmasked void Curves_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  Curves *uniform self = (Curves * uniform) args->geometryUserPtr;
  uniform int primID = args->primID;
  const uniform uint32 index = get_int32(self->index, primID);
  const uniform vec3f A = get_vec3f(self->vertex, index);
  const uniform vec3f B = get_vec3f(self->vertex, index + 1);
  box3fa *uniform out = (box3fa * uniform) args->bounds_o;
  *out = make_box3fa(min(A, B) - self->radius, max(A, B) + self->radius);
}

unmasked void Curves_intersect_kernel(
    const struct RTCIntersectFunctionNArguments *uniform args,
    const uniform bool occlusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex])
    return;

  Curves *uniform self = (Curves * uniform) args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer to a varying ray!
  varying Ray *uniform ray = (varying Ray * uniform) args->rayhit;

  const uniform uint32 index = get_int32(self->index, primID);
  const uniform vec3f A = get_vec3f(self->vertex, index);
  const uniform vec3f B = get_vec3f(self->vertex, index + 1);

  const Intersections isect =
      intersectCapsule(ray->org, ray->dir, A, B, self->radius);

  bool hit = false;
  if (and(and(isect.entry.t > ray->t0, isect.entry.t < ray->t),
          isect.entry.hit)) {
    hit = true;
    ray->t = isect.entry.t;
    ray->u = isect.entry.u;
    ray->Ng = isect.entry.N;
  } else if (and(and(isect.exit.t > ray->t0, isect.exit.t < ray->t),
                 isect.exit.hit)) {
    hit = true;
    ray->t = isect.exit.t;
    ray->u = isect.exit.u;
    ray->Ng = isect.exit.N;
  }

  if (hit) {
    if (occlusionTest) {
      ray->t = neg_inf;
    } else {
      ray->primID = primID;
      ray->geomID = args->geomID;
      ray->instID = args->context->instID[0];
    }
  }
}

unmasked void Curves_intersect(
    const struct RTCIntersectFunctionNArguments *uniform args)
{
  Curves_intersect_kernel(args, false);
}

unmasked void Curves_occluded(
    const struct RTCOccludedFunctionNArguments *uniform args)
{
  Curves_intersect_kernel(
      (RTCIntersectFunctionNArguments * uniform) args, true);
}

// For simplicity streamlines are modelled of segments, each consisting of one
// cylinder and two spheres. Sampling would put (too much) effort towards the
// vertices, where often two spheres and two cylinders overlap. Furthermore,
// sampling such a mixed "primitive" is more complex. Thus, choose a different
// representation for area sampling, i.e. one cylinder per segment and one
// sphere per vertex.
SampleAreaRes Curves_sampleArea(const Geometry *uniform const _self,
    const int32 areaPrimID,
    const uniform affine3f &xfm,
    const uniform affine3f &rcp_xfm,
    const vec2f &s)
{
  const Curves *const uniform self = (const Curves *uniform)_self;
  const uniform float radius = self->radius;

  vec3f localNormal;
  vec3f localPos;
  const uniform int32 spheres = self->vertex.numItems;
  if (areaPrimID < spheres) {
    localNormal = uniformSampleSphere(radius, s);
    const vec3f center = get_vec3f(self->vertex, areaPrimID);
    localPos = center + localNormal;
  } else { // cylinder
    const uint32 idx = get_uint32(self->index, areaPrimID - spheres);
    const vec3f v0 = get_vec3f(self->vertex, idx);
    const vec3f v1 = get_vec3f(self->vertex, idx + 1);

    linear3f frame = frame(v1 - v0);
    frame.vy = normalize(frame.vy);
    const float phi = two_pi * s.x;
    float sinPhi, cosPhi;
    sincos(phi, &sinPhi, &cosPhi);
    localNormal = (cosPhi * radius) * frame.vx + (sinPhi * radius) * frame.vy;
    localPos = v0 + localNormal + s.y * frame.vz;
  }

  SampleAreaRes res;
  res.pos = xfmPoint(xfm, localPos);
  res.normal = normalize(xfmVector(transposed(rcp_xfm.l), localNormal));

  return res;
}

void Curves_getAreas(const Geometry *const uniform _self,
    const int32 *const uniform primIDs,
    const uniform int32 numPrims,
    const uniform affine3f &xfm,
    float *const uniform areas)
{
  Curves *uniform self = (Curves * uniform) _self;

  // detect (non-uniform) scaling; get length of transformed unit-vectors
  const uniform vec3f scaling3 =
      make_vec3f(length(xfm.l.vx), length(xfm.l.vy), length(xfm.l.vz));
  const uniform float min_scaling = reduce_min(scaling3);
  const uniform float max_scaling = reduce_max(scaling3);
  if ((max_scaling - min_scaling) > 1e-4f * min_scaling)
    print(
        "#osp:StreamLines_getAreas: Non-uniform scaling in instance "
        "transformation detected! Importance sampling for emissive "
        "materials and thus resulting image may be wrong.\n");
  // use average as approximation to closest uniform scaling
  const uniform float scaling = reduce_add(scaling3) / 3.f;

  // spheres at vertices
  const uniform float sphereArea = four_pi * sqr(self->radius * scaling);
  const uniform int32 spheres = self->vertex.numItems;

  // cylinders for segments; area of a cylinder with unit-length
  const uniform float cyl1Area = two_pi * self->radius * sqr(scaling);
  // TODO vectorize this loop, with foreach or ProgramCount & ProgramIndex
  for (uniform int32 i = 0; i < numPrims; i++) {
    const uniform int32 primID = primIDs[i];
    if (primID < spheres) { // sphere
      areas[i] = sphereArea;
    } else { // cylinder
      const uniform uint32 idx = get_uint32(self->index, primID - spheres);
      const uniform vec3f v0 = get_vec3f(self->vertex, idx);
      const uniform vec3f v1 = get_vec3f(self->vertex, idx + 1);
      areas[i] = cyl1Area * length(v0 - v1);
    }
  }
}

void Curves_Constructor(Curves *uniform self, void *uniform cppEquivalent)
{
  Geometry_Constructor(&self->super, cppEquivalent, NULL);
  Data1D_Constructor(&self->index);
  Data1D_Constructor(&self->vertex);
  Data1D_Constructor(&self->color);
  Data1D_Constructor(&self->texcoord);
}

export void *uniform Curves_create(void *uniform cppEquivalent)
{
  Curves *uniform self = uniform new Curves;
  Curves_Constructor(self, cppEquivalent);
  return self;
}

export void *uniform Curves_set(void *uniform _self,
    RTCGeometry geom,
    uniform bool haveColor,
    uniform bool haveTexture,
    int32 uniform numSegments)
{
  Curves *uniform self = (Curves * uniform) _self;
  self->geom = geom;
  self->flagMask = -1;
  if (!haveColor)
    self->flagMask &= ~DG_COLOR;
  if (!haveTexture)
    self->flagMask &= ~DG_TEXCOORD;

  self->super.numPrimitives = numSegments;
  self->super.postIntersect = Curves_postIntersect;

  // TODO implement area sampling of OldCurves for geometry lights
  self->super.getAreas = NULL;
  self->super.sampleArea = NULL;
}

export void *uniform Curves_setUserGeometry(void *uniform _self,
    void *uniform _embreeGeometry,
    float uniform radius,
    const Data1D *uniform index,
    const Data1D *uniform vertex,
    const Data1D *uniform color,
    const Data1D *uniform texcoord)
{
  Curves *uniform self = (Curves * uniform) _self;

  self->radius = radius;
  self->index = *index;
  self->vertex = *vertex;
  self->color = *color;
  self->texcoord = *texcoord;

  self->super.getAreas = Curves_getAreas;
  self->super.sampleArea = Curves_sampleArea;
  self->super.postIntersect = CurvesUserGeometry_postIntersect;
  self->super.numPrimitives = index->numItems;

  Geometry_setEmbreeUserGeometry(&self->super,
      (RTCGeometry)_embreeGeometry,
      Curves_bounds,
      Curves_intersect,
      Curves_occluded);

  // XXX different representation for area sampling, see above
  self->super.numPrimitives += vertex->numItems;
}
