// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "common/sg/SceneGraph.h"
#include "sg/geometry/TriangleMesh.h"

#include "ospapp/OSPApp.h"
#include "widgets/imguiViewer.h"

namespace ospray {
  namespace app {

    class OSPExampleViewer : public OSPApp
    {
      void render(const std::shared_ptr<sg::Frame> &) override;
      int parseCommandLine(int &ac, const char **&av) override;

      bool fullscreen = false;
      float motionSpeed = -1.f;
      std::string initialTextForNodeSearch;
    };

    void OSPExampleViewer::render(const std::shared_ptr<sg::Frame> &root)
    {
      ospray::ImGuiViewer window(root);

      window.create("OSPRay Example Viewer App",
                    fullscreen, vec2i(width, height));

      if (motionSpeed > 0.f)
        window.setMotionSpeed(motionSpeed);

      if (!initialTextForNodeSearch.empty())
        window.setInitialSearchBoxText(initialTextForNodeSearch);

      window.setColorMap(defaultTransferFunction);

      // emulate former negative spp behavior
      auto &renderer = root->child("renderer");
      int spp = renderer["spp"].valueAs<int>();
      if (spp < 1) {
        window.navRenderResolutionScale = ::powf(2, spp);
        renderer["spp"] = 1;
      }

      imgui3D::run();
    }

    int OSPExampleViewer::parseCommandLine(int &ac, const char **&av)
    {
      for (int i = 1; i < ac; i++) {
        const std::string arg = av[i];
        if (arg == "--fullscreen") {
          fullscreen = true;
          removeArgs(ac, av, i, 1);
          --i;
        } else if (arg == "--motionSpeed") {
          motionSpeed = atof(av[i + 1]);
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "--searchText") {
          initialTextForNodeSearch = av[i + 1];
          removeArgs(ac, av, i, 2);
          --i;
        }
      }
      return 0;
    }

  } // ::ospray::app
} // ::ospray

int main(int ac, const char **av)
{
  ospray::app::OSPExampleViewer ospApp;
  return ospApp.main(ac, av);
}
