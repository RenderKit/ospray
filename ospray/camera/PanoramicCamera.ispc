// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.ih"
#include "math/sampling.ih"
#include "ospray/OSPEnums.h"

struct PanoramicCamera
{
  Camera super;

  int stereoMode;
  float ipd_offset; // half of the interpupillary distance
};

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

  // transform to camera- and then to world-space
  const vec3f dir =
      self->super.frame * make_vec3f(-localDir.y, -localDir.z, localDir.x);

  const float time = Camera_shutterTime(_self, sample.time);
  const vec3f org = self->super.org + offset * cross(dir, self->super.frame.vy);
  setRay(ray, org, dir, self->super.nearClip, inf, time);
}

export void *uniform PanoramicCamera_create(void *uniform cppE)
{
  PanoramicCamera *uniform self = uniform new PanoramicCamera;
  self->super.cppEquivalent = cppE;
  self->super.initRay = PanoramicCamera_initRay;
  self->super.doesDOF = false;
  self->stereoMode = OSP_STEREO_NONE;
  self->ipd_offset = 0.f;
  return self;
}

export void PanoramicCamera_set(
    void *uniform _self, const uniform int stereoMode, const uniform float ipd)
{
  PanoramicCamera *uniform self = (PanoramicCamera * uniform) _self;
  self->stereoMode = stereoMode;
  self->ipd_offset = 0.5f * ipd;
  // flip offset to have left eye at top (image coord origin at lower left)
  if (stereoMode == OSP_STEREO_TOP_BOTTOM)
    self->ipd_offset = -self->ipd_offset;
}
