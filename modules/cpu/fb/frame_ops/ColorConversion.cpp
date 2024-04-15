// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ColorConversion.h"
#include "fb/FrameBufferView.h"

DECLARE_FRAMEOP_KERNEL_LAUNCHER(ColorConversion_kernelLauncher);

namespace ospray {

LiveColorConversionFrameOp::LiveColorConversionFrameOp(devicert::Device &device,
    FrameBufferView &fbView,
    OSPFrameBufferFormat targetColorFormat)
    : AddStructShared(device, device, fbView),
      convBuffer(
          device, fbView.viewDims.long_product() * sizeOf(targetColorFormat))
{
  getSh()->targetColorFormat = targetColorFormat;
  getSh()->convBuffer = convBuffer.devicePtr();
}

devicert::AsyncEvent LiveColorConversionFrameOp::process()
{
  const vec2ui &itemDims = getSh()->super.viewDims;
  return device.launchFrameOpKernel(
      itemDims, ispc::ColorConversion_kernelLauncher, &getSh()->super);
}

} // namespace ospray
