// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "PixelOp.h"
#include "common/Library.h"
#include "common/Util.h"
// stl
#include <map>

namespace ospray {

  PixelOp::Instance *PixelOp::createFromInstance(FrameBuffer *fb,
                                                 PixelOp::Instance *prev)
  {
    UNUSED(fb, prev);
    return nullptr;
  }

  PixelOp *PixelOp::createInstance(const char *type)
  {
    return createInstanceHelper<PixelOp, OSP_PIXEL_OP>(type);
  }

  std::string PixelOp::Instance::toString() const
  {
    return "ospray::PixelOp(base class)";
  }
}
