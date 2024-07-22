// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Debug.h"
#include "fb/FrameBufferView.h"

DECLARE_FRAMEOP_KERNEL_LAUNCHER(Debug_kernelLauncher);

namespace ospray {

DebugFrameOp::DebugFrameOp(devicert::Device &device) : FrameOp(device) {}

std::unique_ptr<LiveFrameOpInterface> DebugFrameOp::attach(
    FrameBufferView &fbView)
{
  return rkcommon::make_unique<LiveDebugFrameOp>(device, fbView);
}

std::string DebugFrameOp::toString() const
{
  return "ospray::DebugFrameOp";
}

LiveDebugFrameOp::LiveDebugFrameOp(
    devicert::Device &device, FrameBufferView &fbView)
    : LiveFrameOp(device, fbView)
{}

devicert::AsyncEvent LiveDebugFrameOp::process()
{
  const vec2ui &itemDims = getSh()->viewDims;
  return device.launchFrameOpKernel(
      itemDims, ispc::Debug_kernelLauncher, getSh());
}

} // namespace ospray
