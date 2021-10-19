// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.ih"
#include "common/MotionTransform.ih"

struct OrthographicCamera
{
  Camera super;

  vec3f dir;
  // below are essentially unions: 1. if no motionBlur; 2. if motionBlur
  vec3f org; // lower left position of the camera image plane;
             // origin of camera
  vec3f du_size; // delta of ray origin between two pixels in x;
                 // sensor size (in x and y)
  vec3f dv_up; // delta of ray origin between two pixels in y;
               // up direction of camera
};

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

export void *uniform OrthographicCamera_create()
{
  uniform OrthographicCamera *uniform cam =
      uniform new uniform OrthographicCamera;
  cam->super.initRay = OrthographicCamera_initRay;
  return cam;
}

export void OrthographicCamera_set(void *uniform _self,
    const uniform vec3f &org,
    const uniform vec3f &dir,
    const uniform vec3f &up,
    const uniform vec2f &size)
{
  OrthographicCamera *uniform self = (OrthographicCamera * uniform) _self;

  if (self->super.motionBlur) {
    self->dir = dir;
    self->du_size = make_vec3f(size.x, size.y, 1.0f);
    self->dv_up = up;
    self->org = org;
  } else {
    self->dir = normalize(dir);
    self->du_size = normalize(cross(self->dir, up));
    self->dv_up = cross(self->du_size, self->dir) * size.y;
    self->du_size = self->du_size * size.x;
    self->org = org - 0.5f * self->du_size - 0.5f * self->dv_up; // shift
  }
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
