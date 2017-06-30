// ======================================================================== //
// Copyright 2017 Intel Corporation                                         //
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

#include <iostream>
#include <string>
#include <vector>

// NOTE(jda) - Issues with template type deduction w/ GCC 7.1 and Clang 4.0,
//             thus defining the use of operator<<() from pico_bench here before
//             OSPCommon.h (offending file) gets eventually included through
//             ospray_sg headers. [blech]
template <typename T>
inline void outputStats(const T &stats)
{
  std::cout << stats << std::endl;
}

#include "ospcommon/FileName.h"
#include "ospcommon/utility/SaveImage.h"

#include "common/sg/importer/Importer.h"
#include "common/sg/SceneGraph.h"
#include "common/sg/Renderer.h"

#include "pico_bench/pico_bench.h"

namespace ospray {

  using namespace std::chrono;

  void printUsageAndExit()
  {
    exit(0);
  }

  std::vector<std::string> files;
  std::string imageOutputFile = "";
  int numWarmupFrames = 10;
  int numBenchFrames  = 100;
  bool logFrameTimes = false;
  int width  = 1024;
  int height = 1024;

  void initializeOSPRay(int argc, const char *argv[])
  {
    int init_error = ospInit(&argc, argv);
    if (init_error != OSP_NO_ERROR) {
      std::cerr << "FATAL ERROR DURING INITIALIZATION!" << std::endl;
      std::exit(init_error);
    }

    auto device = ospGetCurrentDevice();
    ospDeviceSetStatusFunc(device,
                           [](const char *msg) { std::cout << msg; });

    ospDeviceSetErrorFunc(device,
                          [](OSPError e, const char *msg) {
                            std::cout << "OSPRAY ERROR [" << e << "]: "
                                      << msg << std::endl;
                            std::exit(1);
                          });
  }


  void parseCommandLine(int argc, const char *argv[])
  {
    if (argc <= 1) {
      printUsageAndExit();
    }

    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "--help") {
        printUsageAndExit();
      } else if (arg == "-i" || arg == "--image") {
        imageOutputFile = argv[++i];
      } else if (arg == "-w" || arg == "--width") {
        width = atoi(argv[++i]);
      } else if (arg == "-h" || arg == "--height") {
        height = atoi(argv[++i]);
      } else if (arg == "-wf" || arg == "--warmup") {
        numWarmupFrames = atoi(argv[++i]);
      } else if (arg == "-bf" || arg == "--bench") {
        numBenchFrames = atoi(argv[++i]);
      } else if (arg == "-lft" || arg == "--log-frame-times") {
        logFrameTimes = true;
      } else if (arg[0] != '-') {
        files.push_back(arg);
      }
    }
  }

  extern "C" int main(int argc, const char *argv[])
  {
    initializeOSPRay(argc, argv);
    parseCommandLine(argc, argv);

    // Setup scene nodes //////////////////////////////////////////////////////

    auto renderer_ptr = sg::createNode("renderer", "Renderer");
    auto &renderer = *renderer_ptr;

    renderer["shadowsEnabled"].setValue(true);
    renderer["aoSamples"].setValue(1);
    renderer["camera"]["fovy"].setValue(60.f);

    auto &lights = renderer["lights"];

    auto &sun = lights.createChild("sun", "DirectionalLight");
    sun["color"].setValue(vec3f(1.f,232.f/255.f,166.f/255.f));
    sun["direction"].setValue(vec3f(0.462f,-1.f,-.1f));
    sun["intensity"].setValue(1.5f);

    auto &bounce = lights.createChild("bounce", "DirectionalLight");
    bounce["color"].setValue(vec3f(127.f/255.f,178.f/255.f,255.f/255.f));
    bounce["direction"].setValue(vec3f(-.93,-.54f,-.605f));
    bounce["intensity"].setValue(0.25f);

    auto &ambient = lights.createChild("ambient", "AmbientLight");
    ambient["intensity"].setValue(0.9f);
    ambient["color"].setValue(vec3f(174.f/255.f,218.f/255.f,255.f/255.f));

    auto &world = renderer["world"];

    for (auto file : files) {
      FileName fn = file;
      if (fn.ext() == "ospsg")
        sg::loadOSPSG(renderer_ptr,fn.str());
      else {
        auto importerNode_ptr = sg::createNode(fn.name(), "Importer");
        auto &importerNode = *importerNode_ptr;
        importerNode["fileName"].setValue(fn.str());
        world += importerNode_ptr;
      }
    }

    // Run benchmark //////////////////////////////////////////////////////////

    auto benchmarker = pico_bench::Benchmarker<milliseconds>{numBenchFrames};

    auto stats = benchmarker([&]() {
      //TODO
      return milliseconds{500};// NOTE(jda) - this getting ignored?
    });

    outputStats(stats);

    return 0;
  }

} // ::ospray
