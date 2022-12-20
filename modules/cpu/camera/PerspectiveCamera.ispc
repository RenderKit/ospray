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
  if (sbs || self->stereoMode == OSP_STEREO_TOP_BOTTOM) {
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
  if (sbs || self->stereoMode == OSP_STEREO_TOP_BOTTOM) {
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

OSPRAY_END_ISPC_NAMESPACE
