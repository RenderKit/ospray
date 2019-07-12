// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "ospcommon/tasking/parallel_for.h"
#include "common/Managed.h"
#include "ImageOp.h"

namespace ospray {

  struct OSPRAY_SDK_INTERFACE DebugFrameOp : public FrameOp
  {
    std::unique_ptr<LiveImageOp> attach(FrameBufferView &fbView) override;

    std::string toString() const override;
  };

  struct OSPRAY_SDK_INTERFACE LiveDebugFrameOp : public LiveFrameOp
  {
    LiveDebugFrameOp(FrameBufferView &fbView);
    void process(const Camera *) override;
  };

  // The blur frame op is a test which applies a Gaussian blur to the frame
  struct OSPRAY_SDK_INTERFACE BlurFrameOp : public FrameOp {
    std::unique_ptr<LiveImageOp> attach(FrameBufferView &fbView) override;

    std::string toString() const override;
  };

  template<typename T>
  struct OSPRAY_SDK_INTERFACE LiveBlurFrameOp : public LiveFrameOp
  {
    LiveBlurFrameOp(FrameBufferView &fbView)
      : LiveFrameOp(fbView)
    {}

    void process(const Camera *) override;
  };

  template<typename T>
  void LiveBlurFrameOp<T>::process(const Camera *)
  {
    // TODO: For SRGBA we actually need to convert to linear before filtering
    T *color = static_cast<T*>(fbView.colorBuffer);
    const int blurRadius = 4;
    // variance = std-dev^2
    const float variance = 9.f;
    std::vector<T> blurScratch(fbView.fbDims.x * fbView.fbDims.y * 4, 0);

    // Blur along X for each pixel
    tasking::parallel_for(fbView.fbDims.x * fbView.fbDims.y,
      [&](int pixelID)
      {
        int i = pixelID % fbView.fbDims.x;
        int j = pixelID / fbView.fbDims.x;
        vec4f result = 0.f;
        float weightSum = 0.f;
        for (int b = -blurRadius; b <= blurRadius; ++b) {
          const int bx = i + b;
          if (bx < 0 || bx >= fbView.fbDims.x) continue;

          float weight = 1.f / std::sqrt(2 * M_PI * variance)
                        * std::exp(b / (2.f * variance));
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
    tasking::parallel_for(fbView.fbDims.x * fbView.fbDims.y,
      [&](int pixelID)
      {
        int i = pixelID % fbView.fbDims.x;
        int j = pixelID / fbView.fbDims.x;
        vec4f result = 0.f;
        float weightSum = 0.f;
        for (int b = -blurRadius; b <= blurRadius; ++b) {
          const int by = j + b;
          if (by < 0 || by >= fbView.fbDims.y) continue;

          float weight = 1.f / std::sqrt(2 * M_PI * variance)
                        * std::exp(b / (2.f * variance));
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

  //! Depth frameop replaces the color data with a normalized depth buffer img
  struct OSPRAY_SDK_INTERFACE DepthFrameOp : public FrameOp {
    std::unique_ptr<LiveImageOp> attach(FrameBufferView &fbView) override;

    std::string toString() const override;
  };

  struct OSPRAY_SDK_INTERFACE LiveDepthFrameOp : public LiveFrameOp
  {
    LiveDepthFrameOp(FrameBufferView &fbView);

    void process(const Camera *) override;
  };


}

