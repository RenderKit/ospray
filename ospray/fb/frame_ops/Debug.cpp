// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Debug.h"
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
// std
#include <algorithm>

namespace ospray {

std::unique_ptr<LiveImageOp> DebugFrameOp::attach(FrameBufferView &fbView)
{
  if (!fbView.colorBuffer) {
    throw std::runtime_error(
        "debug frame operation must be attached to framebuffer with color "
        "data");
  }

  return ospcommon::make_unique<LiveDebugFrameOp>(fbView);
}

std::string DebugFrameOp::toString() const
{
  return "ospray::DebugFrameOp";
}

LiveDebugFrameOp::LiveDebugFrameOp(FrameBufferView &_fbView)
    : LiveFrameOp(_fbView)
{}

void LiveDebugFrameOp::process(const Camera *)
{
  // DebugFrameOp just colors the whole frame with red
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
}

} // namespace ospray
