// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "camera/Camera.h"

namespace ospray {

// Implements a panoramic camera with latitude/longitude mapping
struct OSPRAY_SDK_INTERFACE PanoramicCamera : public Camera
{
  PanoramicCamera();
  virtual ~PanoramicCamera() override = default;

  virtual std::string toString() const override;
  virtual void commit() override;
};

} // namespace ospray
