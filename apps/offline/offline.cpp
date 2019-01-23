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
      void printHelp();

      void writeImage(const std::shared_ptr<sg::Frame> &, const std::string &);
      void clearAccum(const std::shared_ptr<sg::Frame> &root);
      void toggleDenoiser(const std::shared_ptr<sg::Frame> &root);

      std::string imageBaseName = "offline";
      bool optWriteAllImages = true;
      bool optWriteFinalImage = true;
      size_t optDenoiser = 0;
      size_t optMaxFrames = 128;
      float optMinVariance = 2.f;
    };

    OSPOffline::OSPOffline()
    {
    }

    void OSPOffline::printHelp()
    {
      std::cout <<
R"text(
./ospOffline [parameters] [scene_files]

ospOffline specific parameters:
   -i --image [baseFilename] (default 'offline')
       base name of saved images
   -oidn --denoiser [0,1,2] (default 0)
       image denoiser (0 = off, 1 = on, 2 = both)
   -wa --writeall (default)
   -nwa --no-writeall
       write all power of 2 sampled images
   -wf --writefinal (default)
   -nwf --no-writefinal
       write final converged image
   -mf --maxframes [int] (default 128)
       maximum number of frames
   -mv --minvariance [float] (default 2%)
       minimum variance to which image will converge
)text"
      << std::endl;
    }

    // Reset accumulation
    void OSPOffline::clearAccum(const std::shared_ptr<sg::Frame> &root)
    {
      auto &camera = root->child("camera");
      camera.markAsModified();
    }

    // Toggle denoiser state
    // Without knowing the initial state from command line parameters
    // (ie, -sg:useDenoiser)
    void OSPOffline::toggleDenoiser(const std::shared_ptr<sg::Frame> &root)
    {
      auto fb = root->child("frameBuffer").nodeAs<sg::FrameBuffer>();
      if (!fb->hasChild("useDenoiser")) {
        std::cout << "denoiser not available" << std::endl;
        return;
      }

      fb->child("useDenoiser") = !(fb->child("useDenoiser").valueAs<bool>());
    }

    void OSPOffline::writeImage(const std::shared_ptr<sg::Frame> &root,
                                const std::string &suffix = "") {

      auto imageOutputFile = imageBaseName + suffix;

      auto fb = root->child("frameBuffer").nodeAs<sg::FrameBuffer>();
      auto fbSize = fb->size();
      auto mappedFB = fb->map(OSP_FB_COLOR);
      if (fb->format() == OSP_FB_RGBA32F) {
        imageOutputFile += "_D.pfm";
        utility::writePFM(imageOutputFile, fbSize.x, fbSize.y, (vec4f*)mappedFB);
      } else {
        imageOutputFile += ".ppm";
        utility::writePPM(imageOutputFile, fbSize.x, fbSize.y, (uint32_t *)mappedFB);
      }

      std::cout << "saved current frame to '" << imageOutputFile << "'" << std::endl;

      fb->unmap(mappedFB);
    }

    void OSPOffline::render(const std::shared_ptr<sg::Frame> &root)
    {
      // XXX: Only loop over denoiser state *if* denoiser is available
      toggleDenoiser(root); // XXX: testing

      clearAccum(root); // XXX: testing

      std::string suffix;

      size_t frameNum = 1;
      float variance;
      do {
        // use snprintf to pad number with leading 0s for sorting
        char str[16];
        snprintf(str, sizeof(str), "_%4.4lu", frameNum);
        suffix = str;

        root->renderFrame();

        auto renderer = root->child("renderer").nodeAs<sg::Renderer>();
        variance = renderer->getLastVariance();

        // use snprintf to format variance percentage
        snprintf(str, sizeof(str), "_v%2.2f%%", variance);
        suffix += str;

        // Output images for power of 2 samples
        if (optWriteAllImages && (frameNum & (frameNum-1)) == 0)
          writeImage(root, suffix);

      } while ((frameNum++ < optMaxFrames) && (variance > optMinVariance));

      if (optWriteFinalImage) {
        suffix += "_final";
        writeImage(root, suffix);
      }

    }

    int OSPOffline::parseCommandLine(int &ac, const char **&av)
    {
      for (int i = 1; i < ac; i++) {
        const std::string arg = av[i];
        if (arg == "--help") {
          printHelp();
        } else if (arg == "-i" || arg == "--image") {
          imageBaseName = av[i + 1];
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-oidn" || arg == "--denoiser") {
          optDenoiser = atoi(av[i + 1]);
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-wa" || arg == "--writeall") {
          optWriteAllImages = true;
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-nwa" || arg == "--no-writeall") {
          optWriteAllImages = false;
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-wf" || arg == "--writefinal") {
          optWriteFinalImage = true;
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-nwf" || arg == "--no-writefinal") {
          optWriteFinalImage = false;
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-mf" || arg == "--maxframes") {
          optMaxFrames = atoi(av[i + 1]);
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-mv" || arg == "--minvariance") {
          optMinVariance = atoi(av[i + 1]);
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-mv" || arg == "--minvariance") {
          optMinVariance = atoi(av[i + 1]);
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
  ospray::app::OSPOffline ospApp;
  return ospApp.main(ac, av);
}
