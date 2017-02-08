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

#include <chrono>

#include "OSPRayFixture.h"
#include "BenchScriptHandler.h"

#include "commandline/Utility.h"
#include "pico_bench/pico_bench.h"

using std::cout;
using std::endl;
using std::string;

void printUsageAndExit()
{
  cout << "Usage: ospBenchmark [options] model_file" << endl;

  cout << endl << "Args:" << endl;

  cout << endl;
  cout << "    model_file --> Scene used for benchmarking, supported types"
            << " are:" << endl;
  cout << "                   stl, msg, tri, xml, obj, hbp, x3d" << endl;

  cout << endl;
  cout << "Options:" << endl;

  cout << endl;
  cout << "**generic rendering options**" << endl;

  cout << endl;
  cout << "    -i | --image --> Specify the base filename to write the"
       << " framebuffer to a file." << endl;
  cout << "                     If ommitted, no file will be written." << endl;
  cout << "                     NOTE: this option adds '.ppm' to the end of the"
       << " filename" << endl;

  cout << endl;
  cout << "    -w | --width --> Specify the width of the benchmark frame"
       << endl;
  cout << "                     default: 1024" << endl;

  cout << endl;
  cout << "    -h | --height --> Specify the height of the benchmark frame"
       << endl;
  cout << "                      default: 1024" << endl;

  cout << endl;
  cout << "    -r | --renderer --> Specify the renderer to be benchmarked."
       << endl;
  cout << "                        Ex: -r pathtracer" << endl;
  cout << "                        default: ao1" << endl;

  /*
   * TODO: This was never used anyway?
  cout << endl;
  cout << "    -bg | --background --> Specify the background color: R G B"
       << endl;
       */

  cout << endl;
  cout << "    -wf | --warmup --> Specify the number of warmup frames: N"
       << endl;
  cout << "                       default: 10" << endl;

  cout << endl;
  cout << "    -bf | --bench --> Specify the number of benchmark frames: N"
       << endl;
  cout << "                      default: 100" << endl;

  cout << endl;
  cout << "    -lft | --log-frame-times --> Log frame time in ms for every frame rendered"
       << endl;

  cout << endl;
  cout << "**camera rendering options**" << endl;

  cout << endl;
  cout << "    -vp | --eye --> Specify the camera eye as: ex ey ez " << endl;

  cout << endl;
  cout << "    -vi | --gaze --> Specify the camera gaze point as: ix iy iz "
       << endl;

  cout << endl;
  cout << "    -vu | --up --> Specify the camera up as: ux uy uz " << endl;


  cout << endl;
  cout << "**volume rendering options**" << endl;

  cout << endl;
  cout << "    -s | --sampling-rate --> Specify the sampling rate for volumes."
       << endl;
  cout << "                             default: 0.125" << endl;

  cout << endl;
  cout << "    -dr | --data-range --> Specify the data range for volumes."
       << " If not specified, then the min and max data" << endl
       << " values will be used when reading the data into memory." << endl;
  cout << "                           Format: low high" << endl;

  cout << endl;
  cout << "    -tfc | --tf-color --> Specify the next color to in the transfer"
       << " function for volumes. Each entry will add to the total list of"
       << " colors in the order they are specified." << endl;
  cout << "                              Format: R G B A" << endl;
  cout << "                         Value Range: [0,1]" << endl;

  cout << "    -tfs | --tf-scale --> Specify the opacity the transfer function"
       << " will scale to: [0,x] where x is the input value." << endl;
  cout << "                          default: 1.0" << endl;

  cout << "    -tff | --tf-file --> Specify the transfer function file to use"
       << endl;

  cout << endl;
  cout << "    -is | --surface --> Specify an isosurface at value: val "
       << endl;

  cout << endl;
  cout << "    --help --> Print this help text" << endl;
#ifdef OSPRAY_APPS_ENABLE_SCRIPTING
  cout << endl;
  cout << "    --script --> Specify a script file to drive the benchmarker.\n"
       << "                 In a script you can access the parsed world and command\n"
       << "                 line benchmark configuration via the following variables:\n"
       << "                 defaultFixture -> benchmark settings from the commandline\n"
       << "                 m -> world model parsed from command line scene args\n"
       << "                 c -> camera set from command line args\n"
       << "                 r -> renderer set from command line args\n";
#endif

  exit(0);
}

std::string imageOutputFile = "";
std::string scriptFile = "";
size_t numWarmupFrames = 0;
size_t numBenchFrames = 0;
bool logFrameTimes = false;
// This is the fixture setup by the command line arguments
std::shared_ptr<OSPRayFixture> cmdlineFixture;

void parseCommandLine(int argc, const char *argv[])
{
  using namespace ospcommon;
  using namespace ospray::cpp;
  if (argc <= 1) {
    printUsageAndExit();
  }

  int width = 0;
  int height = 0;
  for (int i = 1; i < argc; ++i) {
    string arg = argv[i];
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
    } else if (arg == "--script") {
      scriptFile = argv[++i];
    }
  }

  auto ospObjs = parseWithDefaultParsers(argc, argv);

  std::deque<Model> model;
  Renderer renderer;
  Camera camera;
  std::tie(std::ignore, model, renderer, camera) = ospObjs;
  cmdlineFixture = std::make_shared<OSPRayFixture>(renderer, camera, model[0]);
  if (width > 0 || height > 0) {
    cmdlineFixture->setFrameBuffer(width, height);
  }
  // Set the default warm up and bench frames
  if (numWarmupFrames > 0) {
    cmdlineFixture->defaultWarmupFrames = numWarmupFrames;
  }
  if (numBenchFrames > 0){
    cmdlineFixture->defaultBenchFrames = numBenchFrames;
  }
}

int main(int argc, const char *argv[]) {
  ospInit(&argc, argv);
  parseCommandLine(argc, argv);

  if (scriptFile.empty()) {
    // If we don't have a script do this, otherwise run the script
    // and let it setup bench scenes and benchmrk them and so on
    auto stats = cmdlineFixture->benchmark();
    if (logFrameTimes) {
      for (size_t i = 0; i < stats.size(); ++i) {
        std::cout << stats[i].count() << stats.time_suffix << "\n";
      }
    }
    std::cout << "Frame Time " << stats << "\n"
      << "FPS Statistics:\n"
      << "\tmax: " << 1000.0 / stats.min().count() << " fps\n"
      << "\tmin: " << 1000.0 / stats.max().count() << " fps\n"
      << "\tmedian: " << 1000.0 / stats.median().count() << " fps\n"
      << "\tmean: " << 1000.0 / stats.mean().count() << " fps\n";
  } else {
#ifdef OSPRAY_APPS_ENABLE_SCRIPTING
    // The script will be responsible for setting up the benchmark config
    // and calling `benchmark(N)` to benchmark the scene
    BenchScriptHandler scriptHandler(cmdlineFixture);
    scriptHandler.runScriptFromFile(scriptFile);
#else
    throw std::runtime_error("You must build with OSPRAY_APPS_ENABLE_SCRIPTING=ON "
                             "to use scripting");
#endif
  }

  if (!imageOutputFile.empty()) {
    cmdlineFixture->saveImage(imageOutputFile);
  }
  return 0;
}

