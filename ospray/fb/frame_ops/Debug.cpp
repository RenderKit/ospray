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

// ospray
#include "../FrameBufferView.h"
#include "../ImageOp.h"
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
// std
#include <algorithm>

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

  // Definitions //////////////////////////////////////////////////////////////

  std::unique_ptr<LiveImageOp> DebugFrameOp::attach(FrameBufferView &fbView)
  {
    if (!fbView.colorBuffer) {
      static WarnOnce warn(toString() +
                           " requires color data but the "
                           "framebuffer does not have this channel.");
      throw std::runtime_error(toString() +
                               " cannot be attached to framebuffer "
                               "which does not have color data");
    }
    return ospcommon::make_unique<LiveDebugFrameOp>(fbView);
  }

  std::string DebugFrameOp::toString() const
  {
    return "ospray::DebugFrameOp";
  }

  LiveDebugFrameOp::LiveDebugFrameOp(FrameBufferView &_fbView)
      : LiveFrameOp(_fbView)
  {
  }

  void LiveDebugFrameOp::process(const Camera *)
  {
    // DebugFrameOp just colors the whole frame with red
    tasking::parallel_for(fbView.viewDims.x * fbView.viewDims.y, [&](int i) {
      if (fbView.colorBufferFormat == OSP_FB_RGBA8 ||
          fbView.colorBufferFormat == OSP_FB_SRGBA) {
        uint8_t *pixel = static_cast<uint8_t *>(fbView.colorBuffer) + i * 4;
        pixel[0]       = 255;
      } else {
        float *pixel = static_cast<float *>(fbView.colorBuffer) + i * 4;
        pixel[0]     = 1.f;
      }
    });
  }

  OSP_REGISTER_IMAGE_OP(DebugFrameOp, frame_debug);

}  // namespace ospray
