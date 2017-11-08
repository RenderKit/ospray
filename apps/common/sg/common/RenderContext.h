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

#pragma once

#include "sg/common/Common.h"
#include "sg/common/TimeStamp.h"

namespace ospray {
  namespace sg {

    /*! class that encapsulate all the context/state required for
      rendering any object. note we INTENTIONALLY do not use
      shared_ptrs here because certain nodes want to set these values
      to 'this', which isn't valid for shared_ptrs */
    struct OSPSG_INTERFACE RenderContext
    {
      RenderContext() = default;

      //! create a new context with new transformation matrix
      RenderContext(const RenderContext &other, const affine3f &newXfm);

      // Data members //

      std::shared_ptr<sg::World> world;      //!< world we're rendering into
      OSPModel currentOSPModel{nullptr};
      affine3f currentTransform{ospcommon::one};
      float time {0.f};

      OSPRenderer ospRenderer {nullptr};
      int level {0};


      TimeStamp _MTime;
      TimeStamp _childMTime;
    };

    // Inlined RenderContext definitions //////////////////////////////////////

    inline RenderContext::RenderContext(const RenderContext &other,
                                        const affine3f &newXfm)
      : world(other.world),
        currentOSPModel(nullptr),
        currentTransform(newXfm),
        ospRenderer(nullptr),
        level(0)
    {}

  }// ::ospray::sg
}// ::ospray
