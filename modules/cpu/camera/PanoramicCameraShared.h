// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CameraShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct PanoramicCamera
{
  Camera super;

  vec3f org;
  linear3f frame; // union: precomputed frame; or (xxx, up, -dir) if motion blur
  int stereoMode;
  float ipd_offset; // half of the interpupillary distance

#ifdef __cplusplus
  PanoramicCamera()
      : org(0.f), frame(one), stereoMode(OSP_STEREO_NONE), ipd_offset(0.f)
  {
    super.type = CAMERA_TYPE_PANORAMIC;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
