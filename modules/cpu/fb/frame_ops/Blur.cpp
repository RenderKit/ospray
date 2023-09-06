// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Blur.h"
#include "ISPCDevice.h"
#include "common/ISPCRTBuffers.h"

#ifndef OSPRAY_TARGET_SYCL
extern "C" {
#endif
namespace ispc {
void BlurHorizontal_kernelLauncher(
    const void *, void *, const LiveFrameOp *, void *, void *);
void BlurVertical_kernelLauncher(
    const void *, void *, const LiveFrameOp *, void *, void *);
} // namespace ispc
#ifndef OSPRAY_TARGET_SYCL
}
#endif

namespace ospray {

#include "fb/FrameBufferView.h"

BlurFrameOp::BlurFrameOp(api::Device &device)
    : FrameOp(static_cast<api::ISPCDevice &>(device))
{}

std::unique_ptr<LiveFrameOpInterface> BlurFrameOp::attach(
    FrameBufferView &fbView)
{
  if (!fbView.colorBuffer) {
    throw std::runtime_error(
        "blur frame operation must be attached to framebuffer with color "
        "data");
  }

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
  // Calculate weights
  const float variance = 9.f;
  for (uint32_t i = 0; i <= BLUR_RADIUS; i++) {
    getSh()->weights[i] =
        1.f / std::sqrt(2.f * M_PI * variance) * std::exp(-float(i * i) / (2.f * variance));
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
  vec4f *colorBuffer = getSh()->super.fbView.colorBuffer;
  ispc::BlurHorizontal_kernelLauncher(
      colorBuffer, colorBuffer, &getSh()->super, cmdQueue, waitEvent);
  ispc::BlurVertical_kernelLauncher(
      colorBuffer, colorBuffer, &getSh()->super, cmdQueue, waitEvent);
}

} // namespace ospray
