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

BenchScriptHandler::BenchScriptHandler(std::shared_ptr<OSPRayFixture> fixture)
  : OSPRayScriptHandler(fixture->model.handle(), fixture->renderer.handle(), fixture->camera.handle())
{
  OSPRayFixture &chaiFixture = *fixture;
  scriptEngine().add_global(chaiscript::var(chaiFixture), "cmdlineFixture");
  registerScriptTypes();
  registerScriptFunctions();
}
void BenchScriptHandler::registerScriptFunctions() {
  auto &chai = this->scriptEngine();

  auto loadTransferFunction = [&](const std::string &fname) {
    using namespace ospcommon;
    tfn::TransferFunction fcn(fname);
    ospray::cpp::TransferFunction transferFunction("piecewise_linear");
    auto colorsData = ospray::cpp::Data(fcn.rgbValues.size(), OSP_FLOAT3,
                                        fcn.rgbValues.data());
    transferFunction.set("colors", colorsData);

    const float tf_scale = fcn.opacityScaling;
    // Sample the opacity values, taking 256 samples to match the volume viewer
    // the volume viewer does the sampling a bit differently so we match that
    // instead of what's done in createDefault
    std::vector<float> opacityValues;
    const int N_OPACITIES = 256;
    size_t lo = 0;
    size_t hi = 1;
    for (int i = 0; i < N_OPACITIES; ++i) {
      const float x = float(i) / float(N_OPACITIES - 1);
      float opacity = 0;
      if (i == 0) {
        opacity = fcn.opacityValues[0].y;
      } else if (i == N_OPACITIES - 1) {
        opacity = fcn.opacityValues.back().y;
      } else {
        // If we're over this val, find the next segment
        if (x > fcn.opacityValues[lo].x) {
          for (size_t j = lo; j < fcn.opacityValues.size() - 1; ++j) {
            if (x <= fcn.opacityValues[j + 1].x) {
              lo = j;
              hi = j + 1;
              break;
            }
          }
        }
        const float delta = x - fcn.opacityValues[lo].x;
        const float interval = fcn.opacityValues[hi].x - fcn.opacityValues[lo].x;
        if (delta == 0 || interval == 0) {
          opacity = fcn.opacityValues[lo].y;
        } else {
          opacity = fcn.opacityValues[lo].y + delta / interval
            * (fcn.opacityValues[hi].y - fcn.opacityValues[lo].y);
        }
      }
      opacityValues.push_back(tf_scale * opacity);
    }

    auto opacityValuesData = ospray::cpp::Data(opacityValues.size(),
                                               OSP_FLOAT,
                                               opacityValues.data());
    transferFunction.set("opacities", opacityValuesData);
    transferFunction.set("valueRange", vec2f(fcn.dataValueMin, fcn.dataValueMax));

    transferFunction.commit();
    return transferFunction;
  };

  // load the first volume in the file. This isn't really a great solution for
  // final support of loading data from scripts but will work ok.
  auto loadVolume = [&](const std::string &fname) {
    ospray::importer::Group *imported = ospray::importer::import(fname);
    if (imported->volume.empty()) {
      throw std::runtime_error("Volume file " + fname + " contains no volumes");
    }
    return ospray::cpp::Volume(imported->volume[0]->handle);
  };

  // Get an string environment variable
  auto getEnvString = [](const std::string &var){
    return ospray::getEnvVar<std::string>(var).second;
  };

  chai.add(chaiscript::fun(loadTransferFunction), "loadTransferFunction");
  chai.add(chaiscript::fun(loadVolume), "loadVolume");
  chai.add(chaiscript::fun(getEnvString), "getEnvString");
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
      {chaiscript::fun(static_cast<double (Millis::*)() const>(&Millis::count)), "count"},
      {chaiscript::fun(millisToString), "to_string"}
    }
  );

  auto statsToString = [](const BenchStats &stats) {
    std::stringstream ss;
    ss << stats;
    return ss.str();
  };
  chaiscript::utility::add_class<BenchStats>(*m, "Statistics",
    {},
    {
      // Are these casts even needed?? the functons aren't overloaded.
      {chaiscript::fun(static_cast<Millis (BenchStats::*)(const float) const>(&BenchStats::percentile)), "percentile"},
      {chaiscript::fun(static_cast<void (BenchStats::*)(const float)>(&BenchStats::winsorize)), "winsorize"},
      {chaiscript::fun(static_cast<Millis (BenchStats::*)() const>(&BenchStats::median)), "median"},
      {chaiscript::fun(static_cast<Millis (BenchStats::*)() const>(&BenchStats::median_abs_dev)), "median_abs_dev"},
      {chaiscript::fun(static_cast<Millis (BenchStats::*)() const>(&BenchStats::mean)), "mean"},
      {chaiscript::fun(static_cast<Millis (BenchStats::*)() const>(&BenchStats::std_dev)), "std_dev"},
      {chaiscript::fun(static_cast<Millis (BenchStats::*)() const>(&BenchStats::min)), "min"},
      {chaiscript::fun(static_cast<Millis (BenchStats::*)() const>(&BenchStats::max)), "max"},
      {chaiscript::fun(static_cast<size_t (BenchStats::*)() const>(&BenchStats::size)), "size"},
      {chaiscript::fun(static_cast<const Millis& (BenchStats::*)(size_t) const>(&BenchStats::operator[])), "[]"},
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
      {chaiscript::fun(&OSPRayFixture::setFrameBufferDims), "setFrameBufferDims"},
      {chaiscript::fun(setRenderer), "setRenderer"},
      {chaiscript::fun(setCamera), "setCamera"},
      {chaiscript::fun(setModel), "setModel"}
    }
  );

  chai.add(m);
}

#endif


