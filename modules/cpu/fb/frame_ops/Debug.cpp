// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Debug.h"
#include "fb/FrameBuffer.h"
#include "rkcommon/tasking/parallel_for.h"
// ispc shared
#include "fb/LocalFBShared.h"
// std
#include <algorithm>

namespace ospray {

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
    api::ISPCDevice &device, FrameBufferView &_fbView)
    : LiveFrameOp(device, _fbView)
{}

void LiveDebugFrameOp::process(void *waitEvent, const Camera *)
{
#ifdef OSPRAY_TARGET_SYCL
  const FrameBufferView &fbView = getFBView();
  const size_t numTasks = fbView.viewDims.x * fbView.viewDims.y;
  const sycl::nd_range<1> dispatchRange =
      device.computeDispatchRange(numTasks, 16);
  sycl::event event = device.getSyclQueue().submit([&](sycl::handler &cgh) {
    cgh.parallel_for(dispatchRange, [=](sycl::nd_item<1> taskIndex) {
      uint32_t i = taskIndex.get_global_id(0);
      if (i >= numTasks)
        return;
      if (fbView.colorBufferFormat == OSP_FB_RGBA8
          || fbView.colorBufferFormat == OSP_FB_SRGBA) {
        uint8_t *pixel = static_cast<uint8_t *>(fbView.colorBuffer) + i * 4;
        pixel[0] = 255;
      } else {
        float *pixel = static_cast<float *>(fbView.colorBuffer) + i * 4;
        pixel[0] = 1.f;
      }
    });
  });

  if (!waitEvent)
    event.wait_and_throw();
  else
    *(sycl::event *)waitEvent = event;
#else
  (void)waitEvent;

  // DebugFrameOp just colors the whole frame with red
  const FrameBufferView &fbView = getFBView();
  tasking::parallel_for(fbView.viewDims.x * fbView.viewDims.y, [&](int i) {
    if (fbView.colorBufferFormat == OSP_FB_RGBA8
        || fbView.colorBufferFormat == OSP_FB_SRGBA) {
      uint8_t *pixel = static_cast<uint8_t *>(fbView.colorBuffer) + i * 4;
      pixel[0] = 255;
    } else {
      float *pixel = static_cast<float *>(fbView.colorBuffer) + i * 4;
      pixel[0] = 1.f;
    }
  });
#endif
}

} // namespace ospray
