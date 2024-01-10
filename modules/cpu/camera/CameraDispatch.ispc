// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.ih"
#include "CameraDispatch.ih"
#include "OrthographicCamera.ih"
#include "PanoramicCamera.ih"
#include "PerspectiveCamera.ih"
#include "common/FeatureFlagsEnum.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL void Camera_dispatch_initRay(const Camera *uniform self,
    varying Ray &ray,
    const varying CameraSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);
  if ((self->type == CAMERA_TYPE_PERSPECTIVE)
      && (ffo & FFO_CAMERA_PERSPECTIVE)) {
    if (self->motionBlur && (ffo & FFO_CAMERA_MOTION_BLUR))
      PerspectiveCamera_initRayMB(self, ray, sample, ffh);
    else
      PerspectiveCamera_initRay(self, ray, sample, ffh);
  } else if ((self->type == CAMERA_TYPE_ORTHOGRAPHIC)
      && (ffo & FFO_CAMERA_ORTHOGRAPHIC)) {
    OrthographicCamera_initRay(self, ray, sample, ffh);
  } else if ((self->type == CAMERA_TYPE_PANORAMIC)
      && (ffo & FFO_CAMERA_PANORAMIC)) {
    PanoramicCamera_initRay(self, ray, sample, ffh);
  } else {
#ifndef OSPRAY_TARGET_SYCL
    self->initRay(self, ray, sample);
#endif
  }
}

OSPRAY_END_ISPC_NAMESPACE
