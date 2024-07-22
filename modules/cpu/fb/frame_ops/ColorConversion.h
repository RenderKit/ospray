// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "common/DeviceRTImpl.h"
#include "fb/FrameOp.h"
// ispc shared
#include "ColorConversionShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE LiveColorConversionFrameOp
    : public AddStructShared<LiveFrameOp, ispc::LiveColorConversion>
{
  LiveColorConversionFrameOp(devicert::Device &device,
      FrameBufferView &fbView,
      OSPFrameBufferFormat targetColorFormat);
  devicert::AsyncEvent process() override;

  devicert::BufferDeviceShadowed<uint8_t> &getConvertedBuffer()
  {
    return convBuffer;
  }

 private:
  devicert::BufferDeviceShadowedImpl<uint8_t> convBuffer;
};

} // namespace ospray
