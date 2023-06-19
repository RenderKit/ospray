// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.ih"
#include "common/MotionTransform.ih"
// c++ shared
#include "OrthographicCameraShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL void OrthographicCamera_initRay(const Camera *uniform _self,
    varying Ray &ray,
    const varying CameraSample &sample)
{
  const OrthographicCamera *uniform self =
      (const OrthographicCamera *uniform)_self;

  const vec2f screen = Camera_subRegion(_self, sample.screen);
  const float time = Camera_shutterTime(_self, screen, sample.time);

  vec3f dir;
  vec3f org;
#ifndef OSPRAY_TARGET_SYCL
  if (self->super.motionBlur) {
    const affine3f xfm = getInterpolatedTransform(self->super.geom, time);
    // we cannot just transform the final org & dir, because interpolated
    // transforms can scale (even if original transforms are without scale)
    dir = normalize(xfmVector(xfm, self->dir));
    const vec3f up = xfmVector(xfm, self->dv_up);
    org = xfmPoint(xfm, self->org);

    const vec3f du = normalize(cross(dir, up));
    const vec3f dv = cross(du, dir);
    const float x = (screen.x - 0.5f) * self->du_size.x;
    const float y = (screen.y - 0.5f) * self->du_size.y;
    org = org + x * du + y * dv;
  } else
#endif
  {
    dir = self->dir;
    org = self->org + screen.x * self->du_size + screen.y * self->dv_up;
  }

  setRay(ray, org, dir, self->super.nearClip, inf, time);
}

// Exports (called from C++) //////////////////////////////////////////////////

export void *uniform OrthographicCamera_initRay_addr()
{
  return (void *uniform)OrthographicCamera_initRay;
}

OSPRAY_END_ISPC_NAMESPACE
