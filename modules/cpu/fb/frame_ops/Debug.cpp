// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Debug.h"
#include "ISPCDevice.h"
#include "fb/FrameBufferView.h"

#ifndef OSPRAY_TARGET_SYCL
extern "C" {
#endif
namespace ispc {
void Debug_kernelLauncher(const FrameBufferView *, void *, void *);
}
#ifndef OSPRAY_TARGET_SYCL
}
#endif

namespace ospray {

DebugFrameOp::DebugFrameOp(api::Device &device)
    : FrameOp(static_cast<api::ISPCDevice &>(device))
{}

std::unique_ptr<LiveFrameOpInterface> DebugFrameOp::attach(
    FrameBufferView &fbView)
{
  return rkcommon::make_unique<LiveDebugFrameOp>(device, fbView);
}

std::string DebugFrameOp::toString() const
{
  return "ospray::DebugFrameOp";
}

void LiveDebugFrameOp::process(void *waitEvent)
{
  void *cmdQueue = nullptr;
#ifdef OSPRAY_TARGET_SYCL
  cmdQueue = &device.getSyclQueue();
#endif
  ispc::Debug_kernelLauncher(getSh(), cmdQueue, waitEvent);
}

} // namespace ospray
