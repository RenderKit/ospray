// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Blur.h"
#include "fb/FrameBufferView.h"

DECLARE_FRAMEOP_KERNEL_LAUNCHER(BlurHorizontal_kernelLauncher);
DECLARE_FRAMEOP_KERNEL_LAUNCHER(BlurVertical_kernelLauncher);

namespace ospray {

BlurFrameOp::BlurFrameOp(devicert::Device &device) : FrameOp(device) {}

std::unique_ptr<LiveFrameOpInterface> BlurFrameOp::attach(
    FrameBufferView &fbView)
{
  return rkcommon::make_unique<LiveBlurFrameOp>(device, fbView);
}

std::string BlurFrameOp::toString() const
{
  return "ospray::BlurFrameOp";
}

LiveBlurFrameOp::LiveBlurFrameOp(
    devicert::Device &device, FrameBufferView &fbView)
    : AddStructShared(device, device, fbView),
      scratch(device, fbView.viewDims.long_product())
{
  // Set pointer to scratch buffer
  getSh()->scratchBuffer = scratch.devicePtr();

  // Calculate blur weights
  const float variance = 9.f;
  for (uint32_t i = 0; i <= BLUR_RADIUS; i++) {
    getSh()->weights[i] = 1.f / std::sqrt(2.f * M_PI * variance)
        * std::exp(-float(i * i) / (2.f * variance));
  }
  float weightSum = 0.f;
  for (int32_t i = -BLUR_RADIUS; i <= BLUR_RADIUS; i++)
    weightSum += getSh()->weights[abs(i)];
  for (uint32_t i = 0; i <= BLUR_RADIUS; i++)
    getSh()->weights[i] /= weightSum;
}

devicert::AsyncEvent LiveBlurFrameOp::process()
{
  // Horizontal copying pass
  const vec2ui &itemDims = getSh()->super.viewDims;
  device.launchFrameOpKernel(
      itemDims, ispc::BlurHorizontal_kernelLauncher, &getSh()->super);

  // Vertical in-place pass
  return device.launchFrameOpKernel(
      itemDims, ispc::BlurVertical_kernelLauncher, &getSh()->super);
}

} // namespace ospray
