// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.ih"
#include "common/MotionTransform.ih"
// c++ shared
#include "OrthographicCameraShared.h"

void OrthographicCamera_initRay(const Camera *uniform _self,
    varying Ray &ray,
    const varying CameraSample &sample)
{
  const OrthographicCamera *uniform self =
      (const OrthographicCamera *uniform)_self;

  const vec2f screen = Camera_subRegion(_self, sample.screen);
  const float time = Camera_shutterTime(_self, screen, sample.time);

  vec3f dir;
  vec3f org;
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
  } else {
    dir = self->dir;
    org = self->org + screen.x * self->du_size + screen.y * self->dv_up;
  }

  setRay(ray, org, dir, self->super.nearClip, inf, time);
}

// Exports (called from C++) //////////////////////////////////////////////////

export void *uniform OrthographicCamera_initRay_addr()
{
  return OrthographicCamera_initRay;
}

export void OrthographicCamera_projectBox(
    void *uniform _self, const uniform box3f &box, uniform box3f &projection)
{
  OrthographicCamera *uniform self = (OrthographicCamera * uniform) _self;
  // normalize to image plane size
  const uniform vec3f dun = self->du_size / dot(self->du_size, self->du_size);
  const uniform vec3f dvn = self->dv_up / dot(self->dv_up, self->dv_up);

  vec3f projectedPt = make_vec3f(-1.f, -1.f, 1e20f);
  foreach (i = 0 ... 8) {
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

    // Project the point on to the film plane
    const float depth = dot(p - self->org, self->dir);
    const vec3f screenPt = p - depth * self->dir;
    const vec3f screenDir = screenPt - self->org;
    projectedPt.x = dot(screenDir, dun);
    projectedPt.y = dot(screenDir, dvn);
    projectedPt.z = depth;
  }

  // Find the projection of all points that projected to the screen
  if (projectedPt.z < 1e20f) {
    projection.lower.x = reduce_min(projectedPt.x);
    projection.lower.y = reduce_min(projectedPt.y);
    projection.lower.z = reduce_min(projectedPt.z);

    projection.upper.x = reduce_max(projectedPt.x);
    projection.upper.y = reduce_max(projectedPt.y);
    projection.upper.z = reduce_max(projectedPt.z);
  }
}
