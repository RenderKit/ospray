// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "OrthographicCamera.h"
#include "PanoramicCamera.h"
#include "PerspectiveCamera.h"

namespace ospray {

void registerAllCameras()
{
  Camera::registerType<OrthographicCamera>("orthographic");
  Camera::registerType<PanoramicCamera>("panoramic");
  Camera::registerType<PerspectiveCamera>("perspective");
}

} // namespace ospray
