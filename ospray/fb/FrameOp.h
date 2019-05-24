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

namespace ospray {

  struct FrameBuffer;

  /*! A view into a portion of the frambuffer to run the frame operation on
   */
  struct OSPRAY_SDK_INTERFACE FrameBufferView {
    // The total dimensions of the global framebuffer
    vec2i fbDims;
    // The dimensions of this view of the framebuffer
    vec2i viewDims;
    // The additional halo region pixels included in the view, if requested
    vec2i haloDims;

    OSPFrameBufferFormat colorBufferFormat;
    /*! Color buffer of the image, exact pixel format depends
     * on `colorBufferFormat`
     */
    void  *colorBuffer;
    //! One float per pixel, may be NULL
    float *depthBuffer;
    // TODO: Should we pass the accum and variance buffers?
    //! one RGBA per pixel, may be NULL
    //vec4f *accumBuffer;
    //! one RGBA per pixel, may be NULL
    //vec4f *varianceBuffer;
    //! accumulated world-space normal per pixel
    vec3f *normalBuffer;
    //! accumulated albedo, one RGBF32 per pixel
    vec3f *albedoBuffer;

    //! Convenience method to make a view of the entire framebuffer 
    FrameBufferView(FrameBuffer *fb, OSPFrameBufferFormat colorFormat,
                    void *colorBuffer, float *depthBuffer,
                    vec3f *normalBuffer, vec3f *albedoBuffer);
  };

  /*! TODO docs */
  struct OSPRAY_SDK_INTERFACE FrameOp : public ManagedObject
  {
    virtual ~FrameOp() override = default;

    /*! TODO docs */
    virtual void endFrame(FrameBufferView &fb) {}

    //! \brief common function to help printf-debugging
    /*! Every derived class should override this! */
    virtual std::string toString() const override;

    static FrameOp *createInstance(const char *type);
  };

  /*! \brief registers a internal ospray::<ClassName> renderer under
      the externally accessible name "external_name"

      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      frameop. By having this symbol in the shared lib ospray can
      later on always get a handle to this fct and create an instance
      of this frameop.
  */
#define OSP_REGISTER_FRAME_OP(InternalClass, external_name) \
  OSP_REGISTER_OBJECT(FrameOp, frame_op, InternalClass, external_name)


  struct OSPRAY_SDK_INTERFACE DebugFrameOp : public FrameOp {
    void endFrame(FrameBufferView &fb) override;

    std::string toString() const override;
  };

  // The blur frame op is a test which applies a Gaussian blur to the frame
  struct OSPRAY_SDK_INTERFACE BlurFrameOp : public FrameOp {
    void endFrame(FrameBufferView &fb) override;
    template<typename T>
    void applyBlur(FrameBufferView &fb, T *color);

    std::string toString() const override;
  };

  template<typename T>
  void BlurFrameOp::applyBlur(FrameBufferView &fb, T *color)
  {
    const int blurRadius = 4;
    // variance = std-dev^2
    const float variance = 9.f;
    std::vector<T> blurScratch(fb.fbDims.x * fb.fbDims.y * 4, 0);

    // Blur along X for each pixel
    tasking::parallel_for(fb.fbDims.x * fb.fbDims.y,
      [&](int pixelID)
      {
        int i = pixelID % fb.fbDims.x;
        int j = pixelID / fb.fbDims.x;
        vec4f result = 0.f;
        float weightSum = 0.f;
        for (int b = -blurRadius; b <= blurRadius; ++b) {
          const int bx = i + b;
          if (bx < 0 || bx >= fb.fbDims.x) continue;

          float weight = 1.f / std::sqrt(2 * M_PI * variance)
                        * std::exp(b / (2.f * variance));
          weightSum += weight;

          // Assumes 4 color channels, which is the case for the OSPRay color
          // buffer types
          for (int c = 0; c < 4; ++c)
            result[c] += color[(j * fb.fbDims.x + bx) * 4 + c] * weight;
          
        }
        for (int c = 0; c < 4; ++c)
          blurScratch[(j * fb.fbDims.x + i) * 4 + c] = result[c] / weightSum;
      });

    // Blur along Y for each pixel
    tasking::parallel_for(fb.fbDims.x * fb.fbDims.y,
      [&](int pixelID)
      {
        int i = pixelID % fb.fbDims.x;
        int j = pixelID / fb.fbDims.x;
        vec4f result = 0.f;
        float weightSum = 0.f;
        for (int b = -blurRadius; b <= blurRadius; ++b) {
          const int by = j + b;
          if (by < 0 || by >= fb.fbDims.y) continue;

          float weight = 1.f / std::sqrt(2 * M_PI * variance)
                        * std::exp(b / (2.f * variance));
          weightSum += weight;

          // Assumes 4 color channels, which is the case for the OSPRay color
          // buffer types
          for (int c = 0; c < 4; ++c)
            result[c] += blurScratch[(by * fb.fbDims.x + i) * 4 + c] * weight;
          
        }
        for (int c = 0; c < 4; ++c)
          color[(j * fb.fbDims.x + i) * 4 + c] = result[c] / weightSum;
      });
  }

  //! Depth frameop replaces the color data with a normalized depth buffer img
  struct OSPRAY_SDK_INTERFACE DepthFrameOp : public FrameOp {
    void endFrame(FrameBufferView &fb) override;

    std::string toString() const override;
  };
}

