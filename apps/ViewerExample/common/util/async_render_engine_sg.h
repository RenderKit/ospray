// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
  class async_render_engine_sg : public async_render_engine
  {
  public:

    async_render_engine_sg() : lastRTime(0) {};
    async_render_engine_sg(sg::NodeH sgRenderer) : scenegraph(sgRenderer), lastRTime(0) {};
    ~async_render_engine_sg() { stop(); }
    virtual void start(int numThreads= -1) override;
    virtual void stop() override;

    void setRenderer(sg::NodeH n) { scenegraph = n; }

  protected:

  	virtual void run();
  	virtual void validate();

    bool paused={false};
    int runningThreads={0};

    sg::NodeH scenegraph;
    sg::TimeStamp lastRTime;
  };
}
}
