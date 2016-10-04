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

#ifdef OSPRAY_APPS_ENABLE_SCRIPTING

#include <sstream>
#include <string>

#include <ospcommon/vec.h>
#include <ospray/common/OSPCommon.h>

#include "pico_bench/pico_bench.h"
#include "chaiscript/utility/utility.hpp"

#include "tfn_lib/tfn_lib.h"
#include "common/importer/Importer.h"

#include "BenchScriptHandler.h"

BenchScriptHandler::BenchScriptHandler(std::shared_ptr<OSPRayFixture> &fixture)
  : OSPRayScriptHandler(fixture->model.handle(), fixture->renderer.handle(), fixture->camera.handle())
{
  scriptEngine().add_global(chaiscript::var(fixture), "defaultFixture");
  registerScriptTypes();
  registerScriptFunctions();
}
void BenchScriptHandler::registerScriptFunctions() {
  auto &chai = this->scriptEngine();

  // load the first volume in the file. This isn't really a great solution for
  // final support of loading data from scripts but will work ok.
  auto loadVolume = [&](const std::string &fname) {
    ospray::importer::Group *imported = ospray::importer::import(fname);
    if (imported->volume.empty()) {
      throw std::runtime_error("Volume file " + fname + " contains no volumes");
    }
    return ospray::cpp::Volume(imported->volume[0]->handle);
  };

  chai.add(chaiscript::fun(loadVolume), "loadVolume");
}
void BenchScriptHandler::registerScriptTypes() {
  using Millis = OSPRayFixture::Millis;
  auto &chai = this->scriptEngine();
  chaiscript::ModulePtr m = chaiscript::ModulePtr(new chaiscript::Module());

  auto millisToString = [](const Millis &m) {
    return std::to_string(m.count()) + "ms";
  };
  chaiscript::utility::add_class<Millis>(*m, "Millis",
    {},
    {
      {chaiscript::fun(&Millis::count), "count"},
      {chaiscript::fun(millisToString), "to_string"}
    }
  );

  auto statsToString = [](const BenchStats &stats) {
    std::stringstream ss;
    ss << "Frame Time " << stats
      << "\nFPS Statistics:\n"
      << "\tmax: " << 1000.0 / stats.min().count() << " fps\n"
      << "\tmin: " << 1000.0 / stats.max().count() << " fps\n"
      << "\tmedian: " << 1000.0 / stats.median().count() << " fps\n"
      << "\tmean: " << 1000.0 / stats.mean().count() << " fps\n";
    return ss.str();
  };
  chaiscript::utility::add_class<BenchStats>(*m, "Statistics",
    {},
    {
      // Are these casts even needed?? the functons aren't overloaded.
      {chaiscript::fun(static_cast<Millis (BenchStats::*)(const float) const>(&BenchStats::percentile)), "percentile"},
      {chaiscript::fun(static_cast<void (BenchStats::*)(const float)>(&BenchStats::winsorize)), "winsorize"},
      {chaiscript::fun(&BenchStats::median), "median"},
      {chaiscript::fun(&BenchStats::median_abs_dev), "median_abs_dev"},
      {chaiscript::fun(&BenchStats::mean), "mean"},
      {chaiscript::fun(&BenchStats::std_dev), "std_dev"},
      {chaiscript::fun(&BenchStats::min), "min"},
      {chaiscript::fun(&BenchStats::max), "max"},
      {chaiscript::fun(&BenchStats::size), "size"},
      {chaiscript::fun(&BenchStats::operator[]), "[]"},
      {chaiscript::fun(static_cast<BenchStats& (BenchStats::*)(const BenchStats&)>(&BenchStats::operator=)), "="},
      {chaiscript::fun(statsToString), "to_string"}
    }
  );

  auto setRenderer = [&](OSPRayFixture &fix, ospray::cpp::Renderer &r) {
    fix.renderer = r;
  };
  auto setCamera = [&](OSPRayFixture &fix, ospray::cpp::Camera &c) {
    fix.camera = c;
  };
  auto setModel = [&](OSPRayFixture &fix, ospray::cpp::Model &m) {
    fix.model = m;
  };
  auto benchDefault = [&](OSPRayFixture &fix) {
    return fix.benchmark();
  };

  chaiscript::utility::add_class<OSPRayFixture>(*m, "Fixture",
    {
      chaiscript::constructor<OSPRayFixture(ospray::cpp::Renderer, ospray::cpp::Camera, ospray::cpp::Model)>(),
    },
    {
      {chaiscript::fun(&OSPRayFixture::benchmark), "benchmark"},
      {chaiscript::fun(benchDefault), "benchmark"},
      {chaiscript::fun(&OSPRayFixture::saveImage), "saveImage"},
      {chaiscript::fun(&OSPRayFixture::setFrameBuffer), "setFrameBuffer"},
      {chaiscript::fun(setRenderer), "setRenderer"},
      {chaiscript::fun(setCamera), "setCamera"},
      {chaiscript::fun(setModel), "setModel"}
    }
  );

  chai.add(m);
}

#endif


