// ======================================================================== //
// Copyright 2016-2018 Intel Corporation                                    //
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
#include "ospapp/OSPApp.h"
#include "common/sg/SceneGraph.h"
#include "ospcommon/utility/SaveImage.h"

#include "sg/Renderer.h"
#include "sg/common/FrameBuffer.h"

namespace ospray {
  namespace app {

    struct OSPBenchmark : public OSPApp
    {
      OSPBenchmark();

    private:

      void render(const std::shared_ptr<sg::Frame> &) override;
      int parseCommandLine(int &ac, const char **&av) override;

      template <typename T>
      void outputStats(const T &stats);
      size_t numWarmupFrames = 10;
      size_t numBenchFrames = 100;
      std::string imageOutputFile = "";
    };

    OSPBenchmark::OSPBenchmark()
    {
      addPlane = false; // NOTE(jda) - override default behavior, can still
                        //             force on/off via standard command-line
                        //             options
    }

    void OSPBenchmark::render(const std::shared_ptr<sg::Frame> &root)
    {
      for (size_t i = 0; i < numWarmupFrames; ++i)
        root->renderFrame();

      // NOTE(jda) - allow 0 bench frames to enable testing only data load times
      if (numBenchFrames <= 0)
        return;

      auto benchmarker =
          pico_bench::Benchmarker<std::chrono::milliseconds>{ numBenchFrames };

      auto stats = benchmarker([&]() {
        root->renderFrame();
        // TODO: measure just ospRenderFrame() time from within ospray_sg
        // return std::chrono::milliseconds{500};
      });

      if (!imageOutputFile.empty()) {
        auto fb = root->child("frameBuffer").nodeAs<sg::FrameBuffer>();
        auto *srcPB = (const uint32_t *)fb->map();
        utility::writePPM(imageOutputFile + ".ppm", width, height, srcPB);
        fb->unmap(srcPB);
      }

      outputStats(stats);
    }

    int OSPBenchmark::parseCommandLine(int &ac, const char **&av)
    {
      for (int i = 1; i < ac; i++) {
        const std::string arg = av[i];
        if (arg == "-i" || arg == "--image") {
          imageOutputFile = av[i + 1];
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-wf" || arg == "--warmup") {
          numWarmupFrames = atoi(av[i + 1]);
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-bf" || arg == "--bench") {
          numBenchFrames = atoi(av[i + 1]);
          removeArgs(ac, av, i, 2);
          --i;
        }
      }
      return 0;
    }

    // NOTE(jda) - Issues with template type deduction w/ GCC 7.1 and Clang 4.0,
    //             thus defining the use of operator<<() from pico_bench here
    //             before OSPCommon.h (offending file) gets eventually included
    //             through ospray_sg headers. [blech]
    template <typename T>
    void OSPBenchmark::outputStats(const T &stats)
    {
      std::cout << stats << std::endl;
    }

  } // ::ospray::app
} // ::ospray

int main(int ac, const char **av)
{
  ospray::app::OSPBenchmark ospApp;
  return ospApp.main(ac, av);
}
