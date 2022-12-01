// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include "ISPCDevice.h"
#include "ISPCDeviceObject.h"
#include "common/OSPCommon.h"
#include "common/StructShared.h"
// ispc shared
#include "Distribution2DShared.h"

namespace ospray {
struct OSPRAY_SDK_INTERFACE Distribution2D
    : public AddStructShared<ISPCDeviceObject, ispc::Distribution2D>
{
  Distribution2D(const vec2i &size, api::ISPCDevice &device);

  std::unique_ptr<BufferShared<float>> cdf_x;
  std::unique_ptr<BufferShared<float>> cdf_y;
};
} // namespace ospray
