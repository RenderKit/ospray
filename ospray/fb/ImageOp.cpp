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

#include "ImageOp.h"
#include "common/Util.h"
#include "fb/FrameBuffer.h"

namespace ospray {

  LiveImageOp::LiveImageOp(FrameBufferView &_fbView) : fbView(_fbView) {}

  ImageOp *ImageOp::createInstance(const char *type)
  {
    return createInstanceHelper<ImageOp, OSP_IMAGE_OPERATION>(type);
  }

  std::string ImageOp::toString() const
  {
    return "ospray::ImageOp(base class)";
  }

  LiveTileOp::LiveTileOp(FrameBufferView &_fbView) : LiveImageOp(_fbView) {}

  LiveFrameOp::LiveFrameOp(FrameBufferView &_fbView) : LiveImageOp(_fbView) {}

}  // namespace ospray
