// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.ih"
#include "math/sampling.ih"

void PanoramicCamera_initRay(const Camera *uniform self,
    varying Ray &ray,
    const varying CameraSample &sample)
{
  const vec2f screen = Camera_subRegion(self, sample.screen);

  const float phi = two_pi * screen.x;
  const float theta = M_PI * screen.y;

  float sinTheta, cosTheta;
  sincos(theta, &sinTheta, &cosTheta);
  const vec3f localDir = cartesian(phi, sinTheta, cosTheta);

  // trnasform to camera- and then to world-space
  const vec3f dir =
      self->frame * make_vec3f(-localDir.y, -localDir.z, localDir.x);

  const float time = Camera_shutterTime(self, sample.time);
  const vec3f org = self->org;
  setRay(ray, org, dir, self->nearClip, inf, time);
}

/*! create a new ispc-side version of a panoramiccamera - with given
    pointer to the c-class version - and return both class pointer and
    pointer to internal data back via the supplied reference
    parameters */
export void *uniform PanoramicCamera_create(void *uniform cppE)
{
  Camera *uniform cam = uniform new Camera;
  cam->cppEquivalent = cppE;
  cam->initRay = PanoramicCamera_initRay;
  cam->doesDOF = false;
  return cam;
}
