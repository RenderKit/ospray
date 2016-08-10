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

#include "pico_bench/pico_bench.h"
#include "BenchScriptHandler.h"

BenchScriptHandler::BenchScriptHandler(OSPRayFixture *fixture)
  : OSPRayScriptHandler(fixture->model->handle(), fixture->renderer->handle(), fixture->camera->handle())
{
  registerScriptTypes();
  registerScriptFunctions();
}
void BenchScriptHandler::registerScriptFunctions() {
  auto &chai = this->scriptEngine();
  auto benchmark = [&]() {
    auto stats = (*fixture->benchmarker)([&]() {
      fixture->renderer->renderFrame(*(fixture->fb), OSP_FB_COLOR | OSP_FB_ACCUM);
    });
    stats.time_suffix = "ms";
    if (OSPRayFixture::logFrameTimes) {
      for (size_t i = 0; i < stats.size(); ++i) {
        std::cout << stats[i].count() << stats.time_suffix << "\n";
      }
    }
    std::cout << stats << "\n";
  };
  chai.add(chaiscript::fun(benchmark), "benchmark");
}
void BenchScriptHandler::registerScriptTypes() {
}

