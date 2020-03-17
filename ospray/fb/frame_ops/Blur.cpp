// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Blur.h"

namespace ospray {

std::unique_ptr<LiveImageOp> BlurFrameOp::attach(FrameBufferView &fbView)
{
  if (!fbView.colorBuffer) {
    throw std::runtime_error(
        "blur frame operation must be attached to framebuffer with color "
        "data");
  }

  if (fbView.colorBufferFormat == OSP_FB_RGBA8
      || fbView.colorBufferFormat == OSP_FB_SRGBA) {
    return ospcommon::make_unique<LiveBlurFrameOp<uint8_t>>(fbView);
  }

  return ospcommon::make_unique<LiveBlurFrameOp<float>>(fbView);
}

std::string BlurFrameOp::toString() const
{
  return "ospray::BlurFrameOp";
}

} // namespace ospray
