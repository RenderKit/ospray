// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Debug.h"
#include "ISPCDevice.h"

#ifndef OSPRAY_TARGET_SYCL
extern "C" {
#endif
namespace ispc {
void Debug_kernelLauncher(
    const void *, void *, const LiveFrameOp *, void *, void *);
}
#ifndef OSPRAY_TARGET_SYCL
}
#endif

namespace ospray {

#include "fb/FrameBufferView.h"

DebugFrameOp::DebugFrameOp(api::Device &device)
    : FrameOp(static_cast<api::ISPCDevice &>(device))
{}

std::unique_ptr<LiveFrameOpInterface> DebugFrameOp::attach(
    FrameBufferView &fbView)
{
  if (!fbView.colorBuffer) {
    throw std::runtime_error(
        "debug frame operation must be attached to framebuffer with color "
        "data");
  }

  return rkcommon::make_unique<LiveDebugFrameOp>(device, fbView);
}

std::string DebugFrameOp::toString() const
{
  return "ospray::DebugFrameOp";
}

LiveDebugFrameOp::LiveDebugFrameOp(
    api::ISPCDevice &device, FrameBufferView &fbView)
    : LiveFrameOp(device, fbView)
{}

void LiveDebugFrameOp::process(void *waitEvent)
{
  void *cmdQueue = nullptr;
#ifdef OSPRAY_TARGET_SYCL
  cmdQueue = &device.getSyclQueue();
#endif
  vec4f *colorBuffer = getSh()->fbView.colorBuffer;
  ispc::Debug_kernelLauncher(
      colorBuffer, colorBuffer, getSh(), cmdQueue, waitEvent);
}

} // namespace ospray
