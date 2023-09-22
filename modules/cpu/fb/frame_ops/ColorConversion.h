// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "fb/FrameOp.h"
// ispc shared
#include "ColorConversionShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE LiveColorConversionFrameOp
    : public AddStructShared<LiveFrameOp, ispc::LiveColorConversion>
{
  LiveColorConversionFrameOp(api::ISPCDevice &device,
      FrameBufferView &fbView,
      OSPFrameBufferFormat targetColorFormat);
  void process(void *waitEvent) override;

  const BufferDeviceShadowed<uint8_t> &getConvertedBuffer()
  {
    return convBuffer;
  }

 private:
  BufferDeviceShadowed<uint8_t> convBuffer;
};

} // namespace ospray
