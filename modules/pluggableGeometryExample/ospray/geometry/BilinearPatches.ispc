// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "common/Ray.ih"
#include "geometry/Geometry.ih"
#include "math/LinearSpace.ih"
#include "math/box.ih"
#include "math/vec.ih"

/*! input data for a single patch */
struct Patch
{
  vec3f v00, v01, v10, v11;
};

struct BilinearPatches
{
  /*! inherit from "Geometry" class: since ISPC doesn't support
      inheritance we simply put the "parent" class as the first
      member; this way any typecast to the parent class will get the
      right members (including 'virtual' function pointers, etc) */
  Geometry super;

  size_t numPatches;
  Patch *uniform patchArray;
};

static void BilinearPatches_postIntersect(const Geometry *uniform geometry,
    varying DifferentialGeometry &dg,
    const varying Ray &ray,
    uniform int64 flags)
{
  const BilinearPatches *uniform self =
      (const BilinearPatches *uniform)geometry;
  dg.Ng = dg.Ns = normalize(ray.Ng);
}

/*! create a new ispc equivalent for the C++ class (cppEquivalent
    points back, if we ever need this. this returns a void * that the
    c++ side can use to call back to us when needed. Note this return
    values _has_ to be a untyped 'void*' because ISPC cannot currently
    export 'real' class types - with function pointers etcpp - to a c
    header file */
export void *uniform BilinearPatches_create(void *uniform cppEquivalent)
{
  BilinearPatches *uniform self = uniform new uniform BilinearPatches;

  /* recursively call the 'inherited' constructor, and pass our
     'overridden' functions for intersect and postintersect. note we
     do not have to set the 'intersect' and 'occluded' functions
     because we use embree to do the traversal for us (see
     BilinearPatches_set()) below */
  Geometry_Constructor(
      &self->super, cppEquivalent, BilinearPatches_postIntersect);
  return self;
}

/*! to be able to have embree build a data structure over us we have
    to be able to tell it the bounding box for each primitimve. this
    is the callback embree calls for querying the (uniform) bounding
    box for a single given primitmive 'primID' */
unmasked void BilinearPatches_bounds(
    const RTCBoundsFunctionArguments *uniform args)
{
  uniform BilinearPatches *uniform self =
      (uniform BilinearPatches * uniform) args->geometryUserPtr;
  uniform int primID = args->primID;
  Patch *uniform patch = self->patchArray + primID;

  box3fa *uniform out = (box3fa * uniform) args->bounds_o;
  *out = box_extend(box_extend(box_extend(box_extend(make_box3fa_empty(),
                                              make_vec3f(patch->v00)),
                                   make_vec3f(patch->v01)),
                        make_vec3f(patch->v10)),
      make_vec3f(patch->v11));
}

bool recursiveBisection(varying Ray &ray,
    const uniform bool isOcclusionTest,
    const varying LinearSpace3f &frame,
    const uniform vec3f p00,
    const uniform vec3f p01,
    const uniform vec3f p10,
    const uniform vec3f p11,
    const uniform float u0,
    const uniform float u1,
    const uniform float v0,
    const uniform float v1,
    uniform int recursionsLeft)
{
  // compute signs to x plane
  const float x0 = dot(p00 - ray.org, frame.vx);
  const float x1 = dot(p01 - ray.org, frame.vx);
  const float x2 = dot(p10 - ray.org, frame.vx);
  const float x3 = dot(p11 - ray.org, frame.vx);
  const float min_x = min(min(x0, x1), min(x2, x3));
  if (min_x > 0.f)
    return false;
  const float max_x = max(max(x0, x1), max(x2, x3));
  if (max_x < 0.f)
    return false;

  // compute signs to y plane
  const float y0 = dot(p00 - ray.org, frame.vy);
  const float y1 = dot(p01 - ray.org, frame.vy);
  const float y2 = dot(p10 - ray.org, frame.vy);
  const float y3 = dot(p11 - ray.org, frame.vy);
  const float min_y = min(min(y0, y1), min(y2, y3));
  if (min_y > 0.f)
    return false;
  const float max_y = max(max(y0, y1), max(y2, y3));
  if (max_y < 0.f)
    return false;

  // distance test
  const float t0 = dot(p00 - ray.org, frame.vz);
  const float t1 = dot(p01 - ray.org, frame.vz);
  const float t2 = dot(p10 - ray.org, frame.vz);
  const float t3 = dot(p11 - ray.org, frame.vz);
  // const float min_t = min(min(t0,t1),min(t2,t3));
  // if (min_t > 0.f) return false;
  const float max_t = max(max(t0, t1), max(t2, t3));
  if (max_t < 0.f)
    return false;

  const uniform float uc = 0.5f * (u0 + u1);
  const uniform float vc = 0.5f * (v0 + v1);

  if (recursionsLeft == 0) {
    const float t = 0.25 * (t0 + t1 + t2 + t3);
    if (t <= ray.t0 || t >= ray.t)
      return false;

    if (!isOcclusionTest) {
      ray.t = t;
      ray.u = uc;
      ray.v = vc;

      const vec3f pu0 = p00 + ray.u * (p01 - p00);
      const vec3f pu1 = p10 + ray.u * (p11 - p10);

      const vec3f pv0 = p00 + ray.v * (p10 - p00);
      const vec3f pv1 = p01 + ray.v * (p11 - p01);

      ray.Ng = cross(pu1 - pu0, pv1 - pv0);
    }
    return true;
  } else {
    const uniform vec3f p0c = 0.5f * (p01 + p00);
    const uniform vec3f p1c = 0.5f * (p11 + p10);
    const uniform vec3f pc0 = 0.5f * (p10 + p00);
    const uniform vec3f pc1 = 0.5f * (p11 + p01);
    const uniform vec3f pcc = 0.25f * (p00 + p01 + p10 + p11);

    bool hit = false;
    hit |= recursiveBisection(ray,
        isOcclusionTest,
        frame,
        p00,
        p0c,
        pc0,
        pcc,
        u0,
        uc,
        v0,
        vc,
        recursionsLeft - 1);
    hit |= recursiveBisection(ray,
        isOcclusionTest,
        frame,
        pc0,
        pcc,
        p10,
        p1c,
        u0,
        uc,
        vc,
        v1,
        recursionsLeft - 1);

    hit |= recursiveBisection(ray,
        isOcclusionTest,
        frame,
        p0c,
        p01,
        pcc,
        pc1,
        uc,
        u1,
        v0,
        vc,
        recursionsLeft - 1);
    hit |= recursiveBisection(ray,
        isOcclusionTest,
        frame,
        pcc,
        pc1,
        p1c,
        p11,
        uc,
        u1,
        vc,
        v1,
        recursionsLeft - 1);
    return hit;
  }
}

/*! this is the function callback the embree calls to compute a single
  ray-primitive intersection. For sake of simplicity we'll use a very,
  very simple recursive test here - this is neither fast nor elegant,
  but does show that you can do mere arbitrary stuff in a ISPC kernel,
  including 'true' recursion. */
void BilinearPatches_intersect_kernel(
    const RTCIntersectFunctionNArguments *uniform args,
    const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex])
    return;

  uniform BilinearPatches *uniform self =
      (uniform BilinearPatches * uniform) args->geometryUserPtr;
  uniform unsigned int primID = args->primID;

  // this assumes that the args->rayhit is actually a pointer toa varying ray!
  varying Ray *uniform ray = (varying Ray * uniform) args->rayhit;

  Patch *uniform patch = self->patchArray + primID;

  // create a simple coordinate system with x and y directions
  // orthogonal to the ray
  const LinearSpace3f coordSys = frame(ray->dir);

  bool hit = recursiveBisection(*ray,
      isOcclusionTest,
      coordSys,
      patch->v00,
      patch->v01,
      patch->v10,
      patch->v11,
      0.f,
      1.f,
      0.f,
      1.f,
      /* number of bisections */ 10);

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

unmasked void BilinearPatches_intersect(
    const struct RTCIntersectFunctionNArguments *uniform args)
{
  BilinearPatches_intersect_kernel(args, false);
}

unmasked void BilinearPatches_occluded(
    const struct RTCIntersectFunctionNArguments *uniform args)
{
  BilinearPatches_intersect_kernel(args, true);
}

/*! the function called by BilinearPatches::commit() to pass the
  actual user data over. for simplicity we assume that this functoin
  gets called exactly once; in practice a user may also _change_ an
  existing class'es data members, but handling this would make this
  code significantly larger */
export void BilinearPatches_finalize(void *uniform _self,
    void *uniform _embreeGeometry,
    float *uniform patchesDataMemory,
    uniform size_t numPatches)
{
  // first, typecast to our 'real' type. since ispc can't export real
  // types to c we have to pass 'self' in as a void*, and typecast
  // here.
  BilinearPatches *uniform self = (BilinearPatches * uniform) _self;

  // set our internal data.
  self->numPatches = numPatches;
  self->patchArray = (Patch * uniform) patchesDataMemory;

  RTCGeometry geom = (RTCGeometry)_embreeGeometry;

  rtcSetGeometryUserPrimitiveCount(geom, numPatches);

  // set 'us' as user data (this will be the first arg in intersect()
  // and computebounds() callbacks
  rtcSetGeometryUserData(geom, self);

  // set function pointers:
  rtcSetGeometryBoundsFunction(
      geom, (uniform RTCBoundsFunction)&BilinearPatches_bounds, self);
  rtcSetGeometryIntersectFunction(
      geom, (uniform RTCIntersectFunctionN)&BilinearPatches_intersect);
  rtcSetGeometryOccludedFunction(
      geom, (uniform RTCOccludedFunctionN)&BilinearPatches_occluded);
  rtcCommitGeometry(geom);
}
