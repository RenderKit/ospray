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

#include "pico_bench/pico_bench.h"

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
#include "common/sg/Renderer.h"
#include "sg/common/FrameBuffer.h"
#include "common/sg/SceneGraph.h"

namespace ospray {

  using namespace std::chrono;

  void printUsageAndExit()
  {
    std::cout << "TODO: usage description..." << std::endl;
    exit(0);
  }

  static std::vector<std::string> files;
  static std::string imageOutputFile = "";
  static size_t numWarmupFrames = 10;
  static size_t numBenchFrames  = 100;
  static int width  = 1024;
  static int height = 1024;

  static vec3f up;
  static vec3f pos;
  static vec3f gaze;
  static float fovy = 60.f;
  static bool customView = false;

  void initializeOSPRay(int argc, const char *argv[])
  {
    int init_error = ospInit(&argc, argv);
    if (init_error != OSP_NO_ERROR) {
      std::cerr << "FATAL ERROR DURING INITIALIZATION!" << std::endl;
      std::exit(init_error);
    }

    auto device = ospGetCurrentDevice();
    if (device == nullptr) {
      std::cerr << "FATAL ERROR DURING GETTING DEVICE!" << std::endl;
      std::exit(1);
    }


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
    if (argc <= 1)
      printUsageAndExit();

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
      } else if (arg == "-vp" || arg == "--eye") {
        pos.x = atof(argv[++i]);
        pos.y = atof(argv[++i]);
        pos.z = atof(argv[++i]);
        customView = true;
      } else if (arg == "-vu" || arg == "--up") {
        up.x = atof(argv[++i]);
        up.y = atof(argv[++i]);
        up.z = atof(argv[++i]);
        customView = true;
      } else if (arg == "-vi" || arg == "--gaze") {
        gaze.x = atof(argv[++i]);
        gaze.y = atof(argv[++i]);
        gaze.z = atof(argv[++i]);
        customView = true;
      } else if (arg == "-fv" || arg == "--fovy") {
        fovy = atof(argv[++i]);
      } else if (arg[0] != '-') {
        files.push_back(arg);
      }
    }
  }

  extern "C" int main(int argc, const char *argv[])
  {
    initializeOSPRay(argc, argv);

    // access/load symbols/sg::Nodes dynamically
    loadLibrary("ospray_sg");

    parseCommandLine(argc, argv);

    // Setup scene nodes //////////////////////////////////////////////////////

    auto renderer_ptr = sg::createNode("renderer", "Renderer");
    auto &renderer = *renderer_ptr;

    renderer["shadowsEnabled"] = true;
    renderer["aoSamples"] = 1;

    auto &lights = renderer["lights"];

    auto &sun = lights.createChild("sun", "DirectionalLight");
    sun["color"] = vec3f(1.f, 232.f/255.f, 166.f/255.f);
    sun["direction"] = vec3f(0.462f, -1.f, -.1f);
    sun["intensity"] = 1.5f;

    auto &bounce = lights.createChild("bounce", "DirectionalLight");
    bounce["color"] = vec3f(127.f/255.f, 178.f/255.f, 255.f/255.f);
    bounce["direction"] = vec3f(-.93, -.54f, -.605f);
    bounce["intensity"] = 0.25f;

    auto &ambient = lights.createChild("ambient", "AmbientLight");
    ambient["intensity"] = 0.9f;
    ambient["color"] = vec3f(174.f/255.f, 218.f/255.f, 255.f/255.f);

    auto &world = renderer["world"];

    for (auto file : files) {
      FileName fn = file;
      if (fn.ext() == "ospsg")
        sg::loadOSPSG(renderer_ptr,fn.str());
      else {
        auto importerNode_ptr = sg::createNode(fn.name(), "Importer");
        auto &importerNode = *importerNode_ptr;
        importerNode["fileName"] = fn.str();
        world.add(importerNode_ptr);
      }
    }

    auto sgFB = renderer.child("frameBuffer").nodeAs<sg::FrameBuffer>();
    sgFB->child("size") = vec2i(width, height);

    renderer.traverse("verify");
    renderer.traverse("commit");

    // Setup camera ///////////////////////////////////////////////////////////

    if (!customView) {
      auto bbox  = world.bounds();
      vec3f diag = bbox.size();
      diag       = max(diag,vec3f(0.3f*length(diag)));

      gaze = ospcommon::center(bbox);

      pos = gaze - .75f*vec3f(-.6*diag.x,-1.2f*diag.y,.8f*diag.z);
      up  = vec3f(0.f, 1.f, 0.f);
    }

    auto dir = gaze - pos;

    auto &camera = renderer["camera"];
    camera["fovy"] = fovy;
    camera["pos"]  = pos;
    camera["dir"]  = dir;
    camera["up"]   = up;

    renderer.traverse("commit");

    for (size_t i = 0; i < numWarmupFrames; ++i)
      renderer.traverse("render");

    // Run benchmark //////////////////////////////////////////////////////////

    auto benchmarker = pico_bench::Benchmarker<milliseconds>{numBenchFrames};

    auto stats = benchmarker([&]() {
      renderer.traverse("render");
      // TODO: measure just ospRenderFrame() time from within ospray_sg
      // return milliseconds{500};
    });

    // Print results //////////////////////////////////////////////////////////

    if (!imageOutputFile.empty()) {
      auto *srcPB = (const uint32_t*)sgFB->map();
      utility::writePPM(imageOutputFile + ".ppm", width, height, srcPB);
      sgFB->unmap(srcPB);
    }

    outputStats(stats);

    return 0;
  }

} // ::ospray
