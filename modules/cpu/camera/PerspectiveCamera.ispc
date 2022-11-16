// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.ih"
#include "common/MotionTransform.ih"
#include "math/sampling.ih"
#include "ospray/OSPEnums.h"
// c++ shared
#include "PerspectiveCameraShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL void PerspectiveCamera_initRayMB(const Camera *uniform _self,
    varying Ray &ray,
    const varying CameraSample &sample);

SYCL_EXTERNAL void PerspectiveCamera_initRay(const Camera *uniform _self,
    varying Ray &ray,
    const varying CameraSample &sample)
{
#ifndef OSPRAY_TARGET_SYCL
  // Motion blur not supported in SYCL yet, when it is we'd do better calling
  // initRayMB directly instead of calling into this function first to reduce
  // our register usage
  if (_self->motionBlur) {
    PerspectiveCamera_initRayMB(_self, ray, sample);
    return;
  }
#endif

  const PerspectiveCamera *uniform self =
      (const PerspectiveCamera *uniform)_self;

  vec2f screen = sample.screen;
  vec3f org = self->org;

  const uniform bool sbs = self->stereoMode == OSP_STEREO_SIDE_BY_SIDE;
  varying float *uniform split = sbs ? &screen.x : &screen.y;
#ifdef OSPRAY_TARGET_SYCL
  if (sbs || self->stereoMode == OSP_STEREO_TOP_BOTTOM) {
#else
  if (or (sbs, self->stereoMode == OSP_STEREO_TOP_BOTTOM)) {
#endif
    *split *= 2.f;
    if (*split < 1.f) {
      org = org - self->ipd_offset;
    } else {
      org = org + self->ipd_offset;
      *split -= 1.f;
    }
  }

  screen = Camera_subRegion(_self, screen);

  vec3f dir = self->dir_00 + screen.x * self->du_size + screen.y * self->dv_up;

  if (self->scaledAperture > 0.f) {
    const vec3f llp = uniformSampleDisk(self->scaledAperture, sample.lens);
    // transform local lens point to focal plane (dir_XX are prescaled)
    const vec3f lp =
        (llp.x * self->du_size) + ((llp.y * self->aspect) * self->dv_up);
    org = org + lp;
    dir = dir - lp;
  }

  const float time = Camera_shutterTime(_self, screen, sample.time);
  setRay(ray, org, normalize(dir), self->super.nearClip, inf, time);
}

SYCL_EXTERNAL void PerspectiveCamera_initRayMB(const Camera *uniform _self,
    varying Ray &ray,
    const varying CameraSample &sample)
{
  const PerspectiveCamera *uniform self =
      (const PerspectiveCamera *uniform)_self;

  vec2f screen = sample.screen;
  float ipd_offset1 = self->ipd_offset.x;

  const uniform bool sbs = self->stereoMode == OSP_STEREO_SIDE_BY_SIDE;
  varying float *uniform split = sbs ? &screen.x : &screen.y;
#ifdef OSPRAY_TARGET_SYCL
  if (sbs || self->stereoMode == OSP_STEREO_TOP_BOTTOM) {
#else
  if (or (sbs, self->stereoMode == OSP_STEREO_TOP_BOTTOM)) {
#endif
    *split *= 2.f;
    if (*split < 1.f)
      ipd_offset1 = -ipd_offset1;
    else
      *split -= 1.f;
  }

  screen = Camera_subRegion(_self, screen);
  const float time = Camera_shutterTime(_self, screen, sample.time);
  const affine3f xfm = getInterpolatedTransform(self->super.geom, time);

  vec3f org = xfmPoint(xfm, self->org);
  vec3f dir = normalize(xfmVector(xfm, self->dir_00));
  const vec3f up = xfmVector(xfm, self->dv_up);
  vec3f du = normalize(cross(dir, up));
  vec3f dv;
  if (self->du_size.z > 0.f) // architectural: orient img to be parallel to 'up'
    dv = normalize(up);
  else // rotate film to be perpendicular to 'dir'
    dv = cross(du, dir);

  vec3f ipd_offset = ipd_offset1 * du;

  switch (self->stereoMode) {
  case OSP_STEREO_LEFT:
  case OSP_STEREO_TOP_BOTTOM: // flipped to have left eye at top
    org = org - ipd_offset;
    break;
  case OSP_STEREO_RIGHT:
  case OSP_STEREO_SIDE_BY_SIDE:
    org = org + ipd_offset;
    break;
  default:
    break;
  }

  du = du * self->du_size.x;
  dv = dv * self->du_size.y;
  dir = dir - 0.5f * du - 0.5f * dv;

  // prescale to focal plane
  if (self->scaledAperture > 0.f) {
    du = du * self->ipd_offset.y; // focusDistance
    dv = dv * self->ipd_offset.y; // focusDistance
    dir = dir * self->ipd_offset.y; // focusDistance
  }

  dir = dir + screen.x * du + screen.y * dv;

  if (self->scaledAperture > 0.f) {
    const vec3f llp = uniformSampleDisk(self->scaledAperture, sample.lens);
    // transform local lens point to focal plane (dir_XX are prescaled)
    const vec3f lp = (llp.x * du) + ((llp.y * self->aspect) * dv);
    org = org + lp;
    dir = dir - lp;
  }

  setRay(ray, org, normalize(dir), self->super.nearClip, inf, time);
}

// Exports (called from C++) //////////////////////////////////////////////////

export void *uniform PerspectiveCamera_initRay_addr()
{
  return (void *uniform)PerspectiveCamera_initRay;
}

export void *uniform PerspectiveCamera_initRayMB_addr()
{
  return (void *uniform)PerspectiveCamera_initRayMB;
}

export void PerspectiveCamera_projectBox(
    void *uniform _self, const uniform box3f &box, uniform box3f &projection)
{
  PerspectiveCamera *uniform self = (PerspectiveCamera * uniform) _self;
  // The original viewing direction along the camera
  const uniform vec3f dir =
      normalize(self->dir_00 + 0.5f * self->du_size + 0.5f * self->dv_up);
  const uniform vec3f dun = normalize(self->du_size) / self->imgPlaneSize.x;
  const uniform vec3f dvn = normalize(self->dv_up) / self->imgPlaneSize.y;

  vec3f projectedPt = make_vec3f(-1.f, -1.f, 1e20f);
#ifdef OSPRAY_TARGET_SYCL
  for (uint32 i = 0; i < 8; ++i) {
#else
  foreach (i = 0 ... 8) {
#endif
    // Get the point we should be projecting
    vec3f p;
    switch (i) {
    case 0:
      p = box.lower;
      break;
    case 1:
      p.x = box.upper.x;
      p.y = box.lower.y;
      p.z = box.lower.z;
      break;
    case 2:
      p.x = box.upper.x;
      p.y = box.upper.y;
      p.z = box.lower.z;
      break;
    case 3:
      p.x = box.lower.x;
      p.y = box.upper.y;
      p.z = box.lower.z;
      break;
    case 4:
      p.x = box.lower.x;
      p.y = box.lower.y;
      p.z = box.upper.z;
      break;
    case 5:
      p.x = box.upper.x;
      p.y = box.lower.y;
      p.z = box.upper.z;
      break;
    case 6:
      p = box.upper;
      break;
    case 7:
      p.x = box.lower.x;
      p.y = box.upper.y;
      p.z = box.upper.z;
      break;
    }

    // We find the intersection of the ray through the point with the virtual
    // film plane, then find the vector to this point from the origin of the
    // film plane (screenDir) and project this point onto the x/y axes of
    // the plane.
    const vec3f v = p - self->org;
    const vec3f r = normalize(v);
    const float denom = dot(neg(r), neg(dir));
    if (denom != 0.f) {
      float t = 1.f / denom;
      const vec3f screenDir = r * t - self->dir_00;
      projectedPt.x = dot(screenDir, dun);
      projectedPt.y = dot(screenDir, dvn);
      projectedPt.z = signbits(t) ? -length(v) : length(v);
#ifdef OSPRAY_TARGET_SYCL
      // In SYCL/C++ points are computed iteratively and we need to do min/max
      // reduction here
      projection.lower.x = min(projectedPt.x, projection.lower.x);
      projection.lower.y = min(projectedPt.y, projection.lower.y);
      projection.lower.z = min(projectedPt.z, projection.lower.z);

      projection.upper.x = max(projectedPt.x, projection.upper.x);
      projection.upper.y = max(projectedPt.y, projection.upper.y);
      projection.upper.z = max(projectedPt.z, projection.upper.z);
#endif
    }
  }

#ifndef OSPRAY_TARGET_SYCL
  // In ISPC we compute all points in parallel and do a reduction
  // Find the projection of all points that projected to the screen
  projection.lower.x = reduce_min(projectedPt.x);
  projection.lower.y = reduce_min(projectedPt.y);
  projection.lower.z = reduce_min(projectedPt.z);

  projection.upper.x = reduce_max(projectedPt.x);
  projection.upper.y = reduce_max(projectedPt.y);
  projection.upper.z = reduce_max(projectedPt.z);
#endif
  // If some points are behind and some are in front mark the box
  // as covering the full screen
  if (projection.lower.z < 0.f && projection.upper.z > 0.f) {
    projection.lower.x = 0.f;
    projection.lower.y = 0.f;

    projection.upper.x = 1.f;
    projection.upper.y = 1.f;
  }
}

OSPRAY_END_ISPC_NAMESPACE
