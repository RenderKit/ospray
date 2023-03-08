// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "CameraShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct PerspectiveCamera
{
  Camera super;

  vec3f org; // position of camera, already contains shift when
             // STEREO_{LEFT|RIGHT}
  vec3f dir_00; // direction of ray with screenSample=(0,0); scaled to
                // focusDistance if no motionBlur
  // below are essentially unions: 1. if no motionBlur; 2. if motionBlur
  vec3f du_size; // delta of ray direction between two pixels in x, scaled to
                 // focusDistance; sensor size (in x and y) and architectural
  vec3f dv_up; // delta of ray direction between two pixels in y, scaled to
               // focusDistance; up direction of camera
  vec3f ipd_offset; // shift of camera position for left/right eye (only when
                    // SIDE_BY_SIDE or TOP_BOTTOM); (0.5*ipd, focusDistance, 0)

  vec2f imgPlaneSize;

  float scaledAperture; // radius of aperture prescaled to focal plane, i.e.,
                        // divided by horizontal image plane size
  float aspect; // image plane size x / y
  int stereoMode;

#ifdef __cplusplus
  PerspectiveCamera()
      : org(0.f),
        dir_00(0.f),
        du_size(0.f),
        dv_up(0.f),
        ipd_offset(0.f),
        imgPlaneSize(0.f),
        scaledAperture(0.f),
        aspect(1.f),
        stereoMode(OSP_STEREO_NONE)
  {
    super.type = CAMERA_TYPE_PERSPECTIVE;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
