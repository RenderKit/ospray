// ======================================================================== //
// Copyright 2016-2019 Intel Corporation                                    //
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

#include "ospapp/OSPApp.h"
#include "common/sg/SceneGraph.h"
#include "ospcommon/utility/SaveImage.h"

#include "sg/Renderer.h"
#include "sg/common/FrameBuffer.h"

namespace ospray {
  namespace app {

    struct OSPOffline : public OSPApp
    {
      OSPOffline();

    private:

      void render(const std::shared_ptr<sg::Frame> &) override;
      int parseCommandLine(int &ac, const char **&av) override;

      std::string imageOutputFile = "";
    };

    OSPOffline::OSPOffline()
    {
    }

    void OSPOffline::render(const std::shared_ptr<sg::Frame> &root)
    {
      root->renderFrame();

      if (!imageOutputFile.empty()) {
        auto fb = root->child("frameBuffer").nodeAs<sg::FrameBuffer>();
        auto fbSize = fb->size();
        if (0)
          utility::writePPM(imageOutputFile + ".ppm", fbSize.x, fbSize.y, (uint32_t *)fb->map(OSP_FB_COLOR));
        else
          utility::writePFM(imageOutputFile + ".pfm", fbSize.x, fbSize.y, (vec4f*)fb->map(OSP_FB_COLOR));
      }

    }

    int OSPOffline::parseCommandLine(int &ac, const char **&av)
    {
      for (int i = 1; i < ac; i++) {
        const std::string arg = av[i];
        if (arg == "-i" || arg == "--image") {
          imageOutputFile = av[i + 1];
          removeArgs(ac, av, i, 2);
          --i;
        } 
#if 0
        else if (arg == "-wf" || arg == "--warmup") {
          numWarmupFrames = atoi(av[i + 1]);
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-bf" || arg == "--bench") {
          numBenchFrames = atoi(av[i + 1]);
          removeArgs(ac, av, i, 2);
          --i;
        }
#endif
      }
      return 0;
    }

  } // ::ospray::app
} // ::ospray

int main(int ac, const char **av)
{
  ospray::app::OSPOffline ospApp;
  return ospApp.main(ac, av);
}
