// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.ih"
#include "CameraType.ih"
#include "OrthographicCamera.ih"
#include "PanoramicCamera.ih"
#include "PerspectiveCamera.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL __noinline void Camera_dispatch_initRay(const Camera *uniform self,
    varying Ray &ray,
    const varying CameraSample &sample)
{
  switch (self->type) {
  case CAMERA_TYPE_PERSPECTIVE:
    PerspectiveCamera_initRay(self, ray, sample);
    break;
  case CAMERA_TYPE_ORTHOGRAPHIC:
    OrthographicCamera_initRay(self, ray, sample);
    break;
  case CAMERA_TYPE_PANORAMIC:
    PanoramicCamera_initRay(self, ray, sample);
    break;
  default:
    // TODO: No function pointer for DPCPP version because it'll hang.
    // Should we have an error here or still allow it to be called?
#ifndef OSPRAY_TARGET_DPCPP
    self->initRay(self, ray, sample);
    break;
#else
    break;
#endif
  }
}

OSPRAY_END_ISPC_NAMESPACE

