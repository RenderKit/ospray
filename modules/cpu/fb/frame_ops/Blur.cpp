// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Blur.h"

namespace ospray {

std::unique_ptr<LiveFrameOpInterface> BlurFrameOp::attach(
    FrameBufferView &fbView)
{
  if (!fbView.colorBuffer) {
    throw std::runtime_error(
        "blur frame operation must be attached to framebuffer with color "
        "data");
  }

  if (fbView.colorBufferFormat == OSP_FB_RGBA8
      || fbView.colorBufferFormat == OSP_FB_SRGBA) {
    return rkcommon::make_unique<LiveBlurFrameOp<uint8_t>>(device, fbView);
  }

  return rkcommon::make_unique<LiveBlurFrameOp<float>>(device, fbView);
}

std::string BlurFrameOp::toString() const
{
  return "ospray::BlurFrameOp";
}

} // namespace ospray
