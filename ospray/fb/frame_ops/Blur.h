// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "../ImageOp.h"
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
// std
#include <algorithm>

namespace ospray {

// The blur frame op is a test which applies a Gaussian blur to the frame
struct OSPRAY_SDK_INTERFACE BlurFrameOp : public FrameOp
{
  std::unique_ptr<LiveImageOp> attach(FrameBufferView &fbView) override;

  std::string toString() const override;
};

template <typename T>
struct OSPRAY_SDK_INTERFACE LiveBlurFrameOp : public LiveFrameOp
{
  LiveBlurFrameOp(FrameBufferView &_fbView) : LiveFrameOp(_fbView) {}

  void process(const Camera *) override;
};

// Inlined definitions ////////////////////////////////////////////////////////

template <typename T>
inline void LiveBlurFrameOp<T>::process(const Camera *)
{
  // TODO: For SRGBA we actually need to convert to linear before filtering
  T *color = static_cast<T *>(fbView.colorBuffer);
  const int blurRadius = 4;
  // variance = std-dev^2
  const float variance = 9.f;
  std::vector<T> blurScratch(fbView.fbDims.x * fbView.fbDims.y * 4, 0);

  // Blur along X for each pixel
  tasking::parallel_for(fbView.fbDims.x * fbView.fbDims.y, [&](int pixelID) {
    int i = pixelID % fbView.fbDims.x;
    int j = pixelID / fbView.fbDims.x;
    vec4f result = 0.f;
    float weightSum = 0.f;
    for (int b = -blurRadius; b <= blurRadius; ++b) {
      const int bx = i + b;
      if (bx < 0 || bx >= fbView.fbDims.x)
        continue;

      float weight =
          1.f / std::sqrt(2 * M_PI * variance) * std::exp(b / (2.f * variance));
      weightSum += weight;

      // Assumes 4 color channels, which is the case for the OSPRay color
      // buffer types
      for (int c = 0; c < 4; ++c)
        result[c] += color[(j * fbView.fbDims.x + bx) * 4 + c] * weight;
    }
    for (int c = 0; c < 4; ++c)
      blurScratch[(j * fbView.fbDims.x + i) * 4 + c] = result[c] / weightSum;
  });

  // Blur along Y for each pixel
  tasking::parallel_for(fbView.fbDims.x * fbView.fbDims.y, [&](int pixelID) {
    int i = pixelID % fbView.fbDims.x;
    int j = pixelID / fbView.fbDims.x;
    vec4f result = 0.f;
    float weightSum = 0.f;
    for (int b = -blurRadius; b <= blurRadius; ++b) {
      const int by = j + b;
      if (by < 0 || by >= fbView.fbDims.y)
        continue;

      float weight =
          1.f / std::sqrt(2 * M_PI * variance) * std::exp(b / (2.f * variance));
      weightSum += weight;

      // Assumes 4 color channels, which is the case for the OSPRay color
      // buffer types
      for (int c = 0; c < 4; ++c)
        result[c] += blurScratch[(by * fbView.fbDims.x + i) * 4 + c] * weight;
    }
    for (int c = 0; c < 4; ++c)
      color[(j * fbView.fbDims.x + i) * 4 + c] = result[c] / weightSum;
  });
}

} // namespace ospray
