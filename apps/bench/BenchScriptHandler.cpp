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
#include "chaiscript/utility/utility.hpp"
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
    return stats;
  };
  auto printStats = [](const BenchStats &stats) {
    std::cout << stats << "\n";
  };

  chai.add(chaiscript::fun(benchmark), "benchmark");
  chai.add(chaiscript::fun(printStats), "printStats");
}
void BenchScriptHandler::registerScriptTypes() {
  using Millis = OSPRayFixture::Millis;
  auto &chai = this->scriptEngine();
  chaiscript::ModulePtr m = chaiscript::ModulePtr(new chaiscript::Module());

  chaiscript::utility::add_class<Millis>(*m, "Millis",
    {},
    {
      {chaiscript::fun(static_cast<double (Millis::*)() const>(&Millis::count)), "count"}
    }
  );

  chaiscript::utility::add_class<BenchStats>(*m, "Statistics",
    {},
    {
      {chaiscript::fun(static_cast<Millis (BenchStats::*)(const float) const>(&BenchStats::percentile)), "percentile"},
      {chaiscript::fun(static_cast<void (BenchStats::*)(const float)>(&BenchStats::winsorize)), "winsorize"},
      {chaiscript::fun(static_cast<Millis (BenchStats::*)() const>(&BenchStats::median)), "median"},
      {chaiscript::fun(static_cast<Millis (BenchStats::*)() const>(&BenchStats::median_abs_dev)), "median_abs_dev"},
      {chaiscript::fun(static_cast<Millis (BenchStats::*)() const>(&BenchStats::mean)), "mean"},
      {chaiscript::fun(static_cast<Millis (BenchStats::*)() const>(&BenchStats::std_dev)), "std_dev"},
      {chaiscript::fun(static_cast<Millis (BenchStats::*)() const>(&BenchStats::min)), "min"},
      {chaiscript::fun(static_cast<Millis (BenchStats::*)() const>(&BenchStats::max)), "max"},
      {chaiscript::fun(static_cast<size_t (BenchStats::*)() const>(&BenchStats::size)), "size"},
      {chaiscript::fun(static_cast<const Millis& (BenchStats::*)(size_t) const>(&BenchStats::operator[])), "at"}
    }
  );

  chai.add(m);
}

