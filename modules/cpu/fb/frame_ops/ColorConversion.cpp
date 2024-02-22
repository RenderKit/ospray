// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ColorConversion.h"
#include "ISPCDevice.h"
#include "fb/FrameBufferView.h"

#ifndef OSPRAY_TARGET_SYCL
extern "C" {
#endif
namespace ispc {
void ColorConversion_kernelLauncher(const FrameBufferView *, void *, void *);
}
#ifndef OSPRAY_TARGET_SYCL
}
#endif

namespace ospray {

LiveColorConversionFrameOp::LiveColorConversionFrameOp(api::ISPCDevice &device,
    FrameBufferView &fbView,
    OSPFrameBufferFormat targetColorFormat)
    : AddStructShared(device.getIspcrtContext(), device, fbView),
      convBuffer(device.getIspcrtDevice(),
          fbView.viewDims.long_product() * sizeOf(targetColorFormat))
{
  getSh()->targetColorFormat = targetColorFormat;
  getSh()->convBuffer = convBuffer.devicePtr();
}

void LiveColorConversionFrameOp::process(void *waitEvent)
{
  void *cmdQueue = nullptr;
#ifdef OSPRAY_TARGET_SYCL
  cmdQueue = &device.getSyclQueue();
#endif
  ispc::ColorConversion_kernelLauncher(&getSh()->super, cmdQueue, waitEvent);
}

} // namespace ospray
