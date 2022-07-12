// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "camera/Camera.h"
#include "rkcommon/math/box.h"
// ispc shared
#include "PerspectiveCameraShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE PerspectiveCamera
    : public AddStructShared<Camera, ispc::PerspectiveCamera>
{
  PerspectiveCamera(api::ISPCDevice &device);
  virtual ~PerspectiveCamera() override = default;

  virtual std::string toString() const override;
  virtual void commit() override;

  box3f projectBox(const box3f &b) const override;

  // Data members //

  float fovy{60.f};
  float aspect{1.f};
  float apertureRadius{0.f};
  float focusDistance{1.f};
  bool architectural{false}; // orient image plane to be parallel to 'up' and
                             // shift the lens
  OSPStereoMode stereoMode{OSP_STEREO_NONE};
  float interpupillaryDistance{
      0.0635f}; // distance between the two cameras (stereo)
};

} // namespace ospray
