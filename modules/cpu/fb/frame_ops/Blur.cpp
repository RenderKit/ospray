// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Blur.h"
#include "ISPCDevice.h"
#include "fb/FrameBufferView.h"

#ifndef OSPRAY_TARGET_SYCL
extern "C" {
#endif
namespace ispc {
void BlurHorizontal_kernelLauncher(const FrameBufferView *, void *, void *);
void BlurVertical_kernelLauncher(const FrameBufferView *, void *, void *);
} // namespace ispc
#ifndef OSPRAY_TARGET_SYCL
}
#endif

namespace ospray {

BlurFrameOp::BlurFrameOp(api::Device &device)
    : FrameOp(static_cast<api::ISPCDevice &>(device))
{}

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
    api::ISPCDevice &device, FrameBufferView &fbView)
    : AddStructShared(device.getIspcrtContext(), device, fbView),
      scratch(device.getIspcrtDevice(), fbView.viewDims.long_product())
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

void LiveBlurFrameOp::process(void *waitEvent)
{
  void *cmdQueue = nullptr;
#ifdef OSPRAY_TARGET_SYCL
  cmdQueue = &device.getSyclQueue();
#endif
  // Horizontal copying pass
  ispc::BlurHorizontal_kernelLauncher(&getSh()->super, cmdQueue, waitEvent);

  // Vertical in-place pass
  ispc::BlurVertical_kernelLauncher(&getSh()->super, cmdQueue, waitEvent);
}

} // namespace ospray
