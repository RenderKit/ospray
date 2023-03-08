// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CameraShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

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

#ifdef __cplusplus
  OrthographicCamera() : dir(0.f), org(0.f), du_size(0.f), dv_up(0.f)
  {
    super.type = CAMERA_TYPE_ORTHOGRAPHIC;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
