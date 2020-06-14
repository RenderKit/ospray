// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PerspectiveCamera.ih"
#include "math/sampling.ih"
#include "ospray/OSPEnums.h"

void PerspectiveCamera_initRay(const Camera *uniform _self,
    varying Ray &ray,
    const varying CameraSample &sample)
{
  const PerspectiveCamera *uniform self =
      (const PerspectiveCamera *uniform)_self;

  vec2f screen = sample.screen;
  vec3f org = self->org;

  const uniform bool sbs = self->stereoMode == OSP_STEREO_SIDE_BY_SIDE;
  varying float *uniform split = sbs ? &screen.x : &screen.y;
  if (or(sbs, self->stereoMode == OSP_STEREO_TOP_BOTTOM)) {
    *split *= 2.f;
    if (*split < 1.f) {
      org = org - self->ipd_offset;
    } else {
      org = org + self->ipd_offset;
      *split -= 1.f;
    }
  }

  screen = Camera_subRegion(_self, screen);

  vec3f dir = self->dir_00 + screen.x * self->dir_du + screen.y * self->dir_dv;

  if (self->super.doesDOF) {
    const vec3f llp = uniformSampleDisk(self->scaledAperture, sample.lens);
    // transform local lens point to focal plane (dir_XX are prescaled in this
    // case)
    const vec3f lp =
        (llp.x * self->dir_du) + ((llp.y * self->aspect) * self->dir_dv);
    org = org + lp;
    dir = dir - lp;
  }

  const float time = Camera_shutterTime(_self, sample.time);
  setRay(ray, org, normalize(dir), self->super.nearClip, inf, time);
}

/*! create a new ispc-side version of a perspectivecamera - with given
    pointer to the c-class version - and return both class pointer and
    pointer to internal data back via the supplied reference
    parameters */
export void *uniform PerspectiveCamera_create(void *uniform cppE)
{
  uniform PerspectiveCamera *uniform self =
      uniform new uniform PerspectiveCamera;
  self->super.cppEquivalent = cppE;
  self->super.initRay = PerspectiveCamera_initRay;
  self->super.doesDOF = false;
  return self;
}

export void PerspectiveCamera_set(void *uniform _self,
    const uniform vec3f &org,
    const uniform vec3f &dir_00,
    const uniform vec3f &dir_du,
    const uniform vec3f &dir_dv,
    const uniform float scaledAperture,
    const uniform float aspect,
    const uniform int stereoMode,
    const uniform vec3f &ipd_offset)
{
  PerspectiveCamera *uniform self = (PerspectiveCamera * uniform) _self;
  self->org = org;
  self->dir_00 = dir_00;
  self->dir_du = dir_du;
  self->dir_dv = dir_dv;
  self->scaledAperture = scaledAperture;
  self->super.doesDOF = scaledAperture > 0.f;
  self->aspect = aspect;
  self->stereoMode = stereoMode;
  self->ipd_offset = ipd_offset;
  // flip offset to have left eye at top (image coord origin at lower left)
  if (stereoMode == OSP_STEREO_TOP_BOTTOM)
    self->ipd_offset = neg(self->ipd_offset);
}
