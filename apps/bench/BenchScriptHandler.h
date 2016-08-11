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

#ifdef OSPRAY_APPS_ENABLE_SCRIPTING

#include <memory>
#include "common/script/OSPRayScriptHandler.h"
#include "OSPRayFixture.h"

class BenchScriptHandler : public ospray::OSPRayScriptHandler {
  public:
    BenchScriptHandler(std::shared_ptr<OSPRayFixture> &fixture);

  private:
    void registerScriptFunctions();
    void registerScriptTypes();

    using BenchStats = pico_bench::Statistics<OSPRayFixture::Millis>;
};

#endif

