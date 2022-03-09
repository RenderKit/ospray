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
  const float time = Camera_shutterTime(_self, sample.time);

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

export void *uniform OrthographicCamera_create(void *uniform cppE)
{
  uniform OrthographicCamera *uniform cam =
      uniform new uniform OrthographicCamera;
  cam->super.cppEquivalent = cppE;
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
