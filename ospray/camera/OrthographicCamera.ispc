// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "OrthographicCamera.ih"

void OrthographicCamera_initRay(const Camera *uniform _self,
    varying Ray &ray,
    const varying CameraSample &sample)
{
  const OrthographicCamera *uniform self =
      (const OrthographicCamera *uniform)_self;

  const vec2f screen = Camera_subRegion(_self, sample.screen);

  const vec3f dir = self->dir;
  const vec3f org =
      self->pos_00 + screen.x * self->pos_du + screen.y * self->pos_dv;

  const float time = Camera_shutterTime(_self, sample.time);
  setRay(ray, org, dir, self->super.nearClip, inf, time);
}

/*! create a new ispc-side version of a orthographiccamera - with given
    pointer to the c-class version - and return both class pointer and
    pointer to internal data back via the supplied reference
    parameters */
export void *uniform OrthographicCamera_create(void *uniform cppE)
{
  uniform OrthographicCamera *uniform cam =
      uniform new uniform OrthographicCamera;
  cam->super.cppEquivalent = cppE;
  cam->super.initRay = OrthographicCamera_initRay;
  cam->super.doesDOF = false;
  return cam;
}

export void OrthographicCamera_set(void *uniform _self,
    const uniform vec3f &dir,
    const uniform vec3f &pos_00,
    const uniform vec3f &pos_du,
    const uniform vec3f &pos_dv)
{
  uniform OrthographicCamera *uniform self =
      (uniform OrthographicCamera * uniform) _self;
  self->dir = dir;
  self->pos_00 = pos_00;
  self->pos_du = pos_du;
  self->pos_dv = pos_dv;
}
