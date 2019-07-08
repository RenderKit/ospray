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

#include <algorithm>
#include "common/Util.h"
#include "FrameBuffer.h"
#include "FrameOp.h"

namespace ospray {

  FrameBufferView::FrameBufferView(FrameBuffer *fb, OSPFrameBufferFormat colorFormat,
                                   void *colorBuffer, float *depthBuffer,
                                   vec3f *normalBuffer, vec3f *albedoBuffer)
    : fbDims(fb->getNumPixels()),
      viewDims(fbDims),
      haloDims(0),
      colorBufferFormat(colorFormat),
      colorBuffer(colorBuffer),
      depthBuffer(depthBuffer),
      normalBuffer(normalBuffer),
      albedoBuffer(albedoBuffer)
  {}

  std::string FrameOp::toString() const
  {
    return "ospray::FrameOp(base class)";
  }

  void DebugFrameOp::process(FrameBufferView &fb)
  {
    if (!fb.colorBuffer) {
      static WarnOnce warn(toString() + " requires color data but the "
                           "framebuffer does not have this channel.");
      return;
    }

    // DebugFrameOp just colors the whole frame with red
    tasking::parallel_for(fb.viewDims.x * fb.viewDims.y,
      [&](int i)
      {
        if (fb.colorBufferFormat == OSP_FB_RGBA8
            || fb.colorBufferFormat == OSP_FB_SRGBA)
        {
          uint8_t *pixel = static_cast<uint8_t*>(fb.colorBuffer) + i * 4;
          pixel[0] = 255;
        } else {
          float *pixel = static_cast<float*>(fb.colorBuffer) + i * 4;
          pixel[0] = 1.f;
        }
      });
  }

  std::string DebugFrameOp::toString() const
  {
    return "ospray::DebugFrameOp";
  }

  void BlurFrameOp::process(FrameBufferView &fb)
  {
    if (!fb.colorBuffer) {
      static WarnOnce warn(toString() + " requires color data but the "
                           "framebuffer does not have this channel.");
      return;
    }

    if (fb.colorBufferFormat == OSP_FB_RGBA8
        || fb.colorBufferFormat == OSP_FB_SRGBA)
    {
      // TODO: For SRGBA we actually need to convert to linear before filtering
      applyBlur(fb, static_cast<uint8_t*>(fb.colorBuffer));
    } else {
      applyBlur(fb, static_cast<float*>(fb.colorBuffer));
    }
  }

  std::string BlurFrameOp::toString() const
  {
    return "ospray::BlurFrameOp";
  }


  void DepthFrameOp::process(FrameBufferView &fb)
  {
    if (!fb.colorBuffer || !fb.depthBuffer) {
      static WarnOnce warn(toString() + " requires color and depth data but "
                           "the framebuffer does not have these channels.");
      return;
    }

    // First find the min/max depth range to normalize the image,
    // we don't use minmax_element here b/c we don't want inf to be
    // found as the max depth value
    const int numPixels = fb.fbDims.x * fb.fbDims.y;
    vec2f depthRange(std::numeric_limits<float>::infinity(),
                     -std::numeric_limits<float>::infinity());
    for (int i = 0; i < numPixels; ++i)
    {
      if (!std::isinf(fb.depthBuffer[i]))
      {
        depthRange.x = std::min(depthRange.x, fb.depthBuffer[i]);
        depthRange.y = std::max(depthRange.y, fb.depthBuffer[i]);
      }
    }
    const float denom = 1.f / (depthRange.y - depthRange.x);

    tasking::parallel_for(numPixels,
      [&](int px) {
        float normalizedZ = 1.f;
        if (!std::isinf(fb.depthBuffer[px]))
          normalizedZ = (fb.depthBuffer[px] - depthRange.x) * denom;

        for (int c = 0; c < 3; ++c)
        {
          if (fb.colorBufferFormat == OSP_FB_RGBA8
              || fb.colorBufferFormat == OSP_FB_SRGBA)
          {
            uint8_t *cbuf = static_cast<uint8_t *>(fb.colorBuffer);
            cbuf[px * 4 + c] = static_cast<uint8_t>(normalizedZ * 255.f);
            // TODO: For srgb we should do srgb conversion as well i guess
          } else {
            float *cbuf = static_cast<float *>(fb.colorBuffer);
            cbuf[px * 4 + c] = normalizedZ;
          }
        }
        if (fb.albedoBuffer) {
          fb.albedoBuffer[px] = vec3f(normalizedZ);
        }
      });
  }

  std::string DepthFrameOp::toString() const
  {
    return "ospray::DepthFrameOp";
  }

  OSP_REGISTER_IMAGE_OP(DebugFrameOp, frame_debug);
  OSP_REGISTER_IMAGE_OP(BlurFrameOp, frame_blur);
  OSP_REGISTER_IMAGE_OP(DepthFrameOp, frame_depth);


  bool operator==(const FrameBufferView &a, const FrameBufferView &b)
  {
    return a.fbDims == b.fbDims
      && a.viewDims == b.viewDims
      && a.haloDims == b.haloDims
      && a.colorBufferFormat == b.colorBufferFormat
      && a.colorBuffer == b.colorBuffer
      && a.depthBuffer == b.depthBuffer
      && a.normalBuffer == b.normalBuffer
      && a.albedoBuffer == b.albedoBuffer;
  }

  bool operator!=(const FrameBufferView &a, const FrameBufferView &b)
  {
    return !(a == b);
  }
}

