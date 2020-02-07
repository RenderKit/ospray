// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "camera/Camera.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE PerspectiveCamera : public Camera
{
  PerspectiveCamera();
  virtual ~PerspectiveCamera() override = default;

  virtual std::string toString() const override;
  virtual void commit() override;
  virtual ProjectedPoint projectPoint(const vec3f &p) const override;

  // Data members //

  float fovy;
  float aspect;
  float apertureRadius;
  float focusDistance;
  bool architectural; // orient image plane to be parallel to 'up' and shift the
                      // lens
  vec3f dir_00;
  vec3f dir_du;
  vec3f dir_dv;
  vec2f imgPlaneSize;

  OSPStereoMode stereoMode;
  float interpupillaryDistance; // distance between the two cameras (stereo)
};

} // namespace ospray
