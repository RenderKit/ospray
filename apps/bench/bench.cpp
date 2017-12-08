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

#include <sstream>
#include "pico_bench/pico_bench.h"
#include "ospapp/OSPApp.h"
#include "common/sg/SceneGraph.h"
#include "ospcommon/utility/SaveImage.h"
#include "sg/common/FrameBuffer.h"

namespace ospray {
namespace app {
        class OSPBenchmark : public OSPApp {
        public:
		int main(int ac, const char **av);
	private:
		void setupCamera(sg::Node &renderer);
 		int parseCommandLine(int ac, const char **&av);
		template <typename T> void outputStats(const T &stats);	
		size_t numWarmupFrames = 10;
		size_t numBenchFrames = 100;
		std::string imageOutputFile = "";
        };
}
}

using namespace ospray;

int app::OSPBenchmark::main(int ac, const char **av) {
  int initResult = initializeOSPRay(ac, av);
  if(initResult != 0)
    return initResult;

  // access/load symbols/sg::Nodes dynamically
  loadLibrary("ospray_sg");

  if(parseCommandLine(ac, av) != 0)
	return 1;

  auto renderer_ptr = sg::createNode("renderer", "Renderer");
  auto &renderer = *renderer_ptr;

  if (!initialRendererType.empty())
    renderer["rendererType"] = initialRendererType;

  renderer.createChild("animationcontroller", "AnimationController");

  addLightsToScene(renderer, true);
  addImporterNodesToWorld(renderer);
  addAnimatedImporterNodesToWorld(renderer);
  setupCamera(renderer);
  // last, to be able to modify all created SG nodes
  parseCommandLineSG(ac, av, renderer);
    auto sgFB = renderer.child("frameBuffer").nodeAs<sg::FrameBuffer>();
    sgFB->child("size") = vec2i(width, height);
    renderer.traverse("verify");
    renderer.traverse("commit");

    for (size_t i = 0; i < numWarmupFrames; ++i)
      renderer.traverse("render");

    // Run benchmark //////////////////////////////////////////////////////////
    auto benchmarker = pico_bench::Benchmarker<std::chrono::milliseconds>{numBenchFrames};

    auto stats = benchmarker([&]() {
      renderer.traverse("render");
      // TODO: measure just ospRenderFrame() time from within ospray_sg
      // return std::chrono::milliseconds{500};
    });


    if (!imageOutputFile.empty()) {
      auto *srcPB = (const uint32_t*)sgFB->map();
      utility::writePPM(imageOutputFile + ".ppm", width, height, srcPB);
      sgFB->unmap(srcPB);
    }

  // Print results //////////////////////////////////////////////////////////
  outputStats(stats);
  return 0;
}

void app::OSPBenchmark::setupCamera(sg::Node &renderer) {
 /* if (false) {
  auto &world = renderer["world"];
    auto bbox = world.bounds();
    vec3f diag = bbox.size();
    diag = max(diag, vec3f(0.3f * length(diag)));

    gaze = ospcommon::center(bbox);

    pos = gaze - .75f * vec3f(-.6 * diag.x, -1.2f * diag.y, .8f * diag.z);
    up = vec3f(0.f, 1.f, 0.f);
  }*/

  auto dir = gaze - pos;

  auto &camera = renderer["camera"];
  camera["fovy"] = fovy;
  camera["pos"] = pos;
  camera["dir"] = dir;
  camera["up"] = up;

  renderer.traverse("commit");
}

int app::OSPBenchmark::parseCommandLine(int ac, const char **&av) {
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "-i" || arg == "--image") {
      imageOutputFile = av[i+1];
	removeArgs(ac,av,i,2); --i;
    } else if (arg == "-w" || arg == "--width") {
      width = atoi(av[i+1]);
	removeArgs(ac,av,i,2); --i;
    } else if (arg == "-h" || arg == "--height") {
      height = atoi(av[i+1]);
	removeArgs(ac,av,i,2); --i;
    } else if (arg == "-wf" || arg == "--warmup") {
      numWarmupFrames = atoi(av[i+1]);
	removeArgs(ac,av,i,2); --i;
    } else if (arg == "-bf" || arg == "--bench") {
      numBenchFrames = atoi(av[i+1]);
	removeArgs(ac,av,i,2); --i;
    } 
  }
  return app::OSPApp::parseCommandLine(ac, av);
}


// NOTE(jda) - Issues with template type deduction w/ GCC 7.1 and Clang 4.0,
//             thus defining the use of operator<<() from pico_bench here before
//             OSPCommon.h (offending file) gets eventually included through
//             ospray_sg headers. [blech]
template <typename T>
void app::OSPBenchmark::outputStats(const T &stats)
{
  std::cout << stats << std::endl;
}

int main(int ac, const char **av)
{
  app::OSPBenchmark ospApp;
  return ospApp.main(ac, av);
}

