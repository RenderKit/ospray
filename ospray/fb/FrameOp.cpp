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

#include "common/Util.h"
#include "ospcommon/tasking/parallel_for.h"
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
  {
  }

  FrameOp *FrameOp::createInstance(const char *type)
  {
    return createInstanceHelper<FrameOp, OSP_FRAME_OP>(type);
  }

  std::string FrameOp::toString() const
  {
    return "ospray::FrameOp(base class)";
  }

  void DebugFrameOp::endFrame(FrameBufferView &fb)
  {
    if (fb.colorBuffer == nullptr) {
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

  OSP_REGISTER_FRAME_OP(DebugFrameOp, debug);
}

