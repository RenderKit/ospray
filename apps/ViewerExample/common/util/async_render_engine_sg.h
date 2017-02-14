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

// std
#include <atomic>
#include <thread>
#include <vector>

// ospcommon
#include <ospcommon/box.h>

// ospray::cpp
#include "ospray/ospray_cpp/Renderer.h"

// ospImGui util
#include "FPSCounter.h"
#include "transactional_value.h"

#include "sg/common/Node.h"
#include "async_render_engine.h"

namespace ospray {
  namespace sg {

    class OSPRAY_IMGUI_UTIL_INTERFACE async_render_engine_sg
        : public async_render_engine
    {
    public:

      async_render_engine_sg(sg::NodeH sgRenderer);
      ~async_render_engine_sg() = default;

    private:

      void run() override;
      void validate() override;

      sg::NodeH scenegraph;
      sg::TimeStamp lastRTime {0};
    };

  } // ::ospray::sg
} // ::ospray
