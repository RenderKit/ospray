// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.ih"
#include "common/MotionTransform.ih"
#include "math/sampling.ih"
#include "ospray/OSPEnums.h"
// c++ shared
#include "PanoramicCameraShared.h"

void PanoramicCamera_initRay(const Camera *uniform _self,
    varying Ray &ray,
    const varying CameraSample &sample)
{
  const PanoramicCamera *uniform self = (const PanoramicCamera *uniform)_self;

  vec2f screen = sample.screen;

  varying float *uniform split =
      self->stereoMode == OSP_STEREO_SIDE_BY_SIDE ? &screen.x : &screen.y;
  float offset = 0.f;
  switch (self->stereoMode) {
  case OSP_STEREO_LEFT:
    offset = -self->ipd_offset;
    break;
  case OSP_STEREO_RIGHT:
    offset = self->ipd_offset;
    break;
  case OSP_STEREO_SIDE_BY_SIDE:
  case OSP_STEREO_TOP_BOTTOM:
    *split *= 2.f;
    if (*split < 1.f) {
      offset = -self->ipd_offset;
    } else {
      offset = self->ipd_offset;
      *split -= 1.f;
    }
    break;
  }

  screen = Camera_subRegion(_self, screen);

  const float phi = two_pi * screen.x;
  const float theta = M_PI * screen.y;

  float sinTheta, cosTheta;
  sincos(theta, &sinTheta, &cosTheta);
  const vec3f localDir = cartesian(phi, sinTheta, cosTheta);

  const float time = Camera_shutterTime(_self, screen, sample.time);

  // transform to camera- and then to world-space
  vec3f dir;
  vec3f org;
  if (self->super.motionBlur) {
    const affine3f xfm = getInterpolatedTransform(self->super.geom, time);

    // we cannot just transform the final org & dir, because interpolated
    // transforms can scale (even if original transforms are without scale)
    linear3f frameMB;
    frameMB.vz = normalize(xfmVector(xfm, self->frame.vz));
    frameMB.vx = normalize(cross(xfmVector(xfm, self->frame.vy), frameMB.vz));
    frameMB.vy = cross(frameMB.vz, frameMB.vx);

    dir = frameMB * make_vec3f(-localDir.y, -localDir.z, localDir.x);
    org = xfmPoint(xfm, self->org) + offset * cross(dir, frameMB.vy);
  } else {
    dir = self->frame * make_vec3f(-localDir.y, -localDir.z, localDir.x);
    org = self->org + offset * cross(dir, self->frame.vy);
  }

  setRay(ray, org, dir, self->super.nearClip, inf, time);
}

// Exports (called from C++) //////////////////////////////////////////////////

export void *uniform PanoramicCamera_initRay_addr()
{
  return PanoramicCamera_initRay;
}
