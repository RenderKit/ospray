// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Depth.h"
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
// std
#include <algorithm>

namespace ospray {

std::unique_ptr<LiveImageOp> DepthFrameOp::attach(FrameBufferView &fbView)
{
  if (!fbView.colorBuffer) {
    throw std::runtime_error(
        "depth frame operation must be attached to framebuffer with color "
        "data");
  }

  if (!fbView.depthBuffer) {
    throw std::runtime_error(
        "depth frame operation must be attached to framebuffer with depth "
        "data");
  }

  return ospcommon::make_unique<LiveDepthFrameOp>(fbView);
}

std::string DepthFrameOp::toString() const
{
  return "ospray::DepthFrameOp";
}

LiveDepthFrameOp::LiveDepthFrameOp(FrameBufferView &_fbView)
    : LiveFrameOp(_fbView)
{}

void LiveDepthFrameOp::process(const Camera *)
{
  // First find the min/max depth range to normalize the image,
  // we don't use minmax_element here b/c we don't want inf to be
  // found as the max depth value
  const int numPixels = fbView.fbDims.x * fbView.fbDims.y;
  vec2f depthRange(std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity());
  for (int i = 0; i < numPixels; ++i) {
    if (!std::isinf(fbView.depthBuffer[i])) {
      depthRange.x = std::min(depthRange.x, fbView.depthBuffer[i]);
      depthRange.y = std::max(depthRange.y, fbView.depthBuffer[i]);
    }
  }
  const float denom = 1.f / (depthRange.y - depthRange.x);

  tasking::parallel_for(numPixels, [&](int px) {
    float normalizedZ = 1.f;
    if (!std::isinf(fbView.depthBuffer[px]))
      normalizedZ = (fbView.depthBuffer[px] - depthRange.x) * denom;

    for (int c = 0; c < 3; ++c) {
      if (fbView.colorBufferFormat == OSP_FB_RGBA8
          || fbView.colorBufferFormat == OSP_FB_SRGBA) {
        uint8_t *cbuf = static_cast<uint8_t *>(fbView.colorBuffer);
        cbuf[px * 4 + c] = static_cast<uint8_t>(normalizedZ * 255.f);
      } else {
        float *cbuf = static_cast<float *>(fbView.colorBuffer);
        cbuf[px * 4 + c] = normalizedZ;
      }
    }
    if (fbView.albedoBuffer) {
      fbView.albedoBuffer[px] = vec3f(normalizedZ);
    }
  });
}

} // namespace ospray
