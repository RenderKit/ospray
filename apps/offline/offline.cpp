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

      void writeImage(const std::shared_ptr<sg::FrameBuffer> &, const std::string &);

      std::string imageBaseName = "offline";
      bool optWriteAllImages = true;
      bool optWriteFinalImage = true;
      int optDenoiser = 0;
      size_t optMaxSamples = 1024;
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
   -i    --image [baseFilename] (default 'offline')
           base name of saved images
   -oidn --denoiser [0,1,2] (default 0)
           image denoiser (0 = off, 1 = on, 2 = generate both)
   -wa   --writeall (default)
   -nwa  --no-writeall
           write (or not) all power of 2 sampled images
   -wf   --writefinal (default)
   -nwf  --no-writefinal
           write (or not) final converged image
   -mf   --maxframes [int] (default 1024)
           maximum number of frames
   -mv   --minvariance [float] (default 2%)
           minimum variance to which image will converge
)text"
      << std::endl;
    }

    void OSPOffline::writeImage(const std::shared_ptr<sg::FrameBuffer> &fb,
                                const std::string &suffix = "") {
      auto imageOutputFile = imageBaseName + suffix;
      auto fbSize = fb->size();
      auto mappedFB = fb->map(OSP_FB_COLOR);
      if (fb->format() == OSP_FB_RGBA32F) {
        imageOutputFile += ".pfm";
        utility::writePFM(imageOutputFile, fbSize.x, fbSize.y, (vec4f*)mappedFB);
      } else {
        imageOutputFile += ".ppm";
        utility::writePPM(imageOutputFile, fbSize.x, fbSize.y, (uint32_t *)mappedFB);
      }
      fb->unmap(mappedFB);

      std::cout << "saved current frame to '" << imageOutputFile << "'" << std::endl;
    }

    void OSPOffline::render(const std::shared_ptr<sg::Frame> &root)
    {
      auto renderer = root->child("renderer").nodeAs<sg::Renderer>();
      auto fb = root->child("frameBuffer").nodeAs<sg::FrameBuffer>();
      // disable use of "navFrameBuffer" for first frame
      root->child("navFrameBuffer")["size"] = fb->size();
      auto &camera = root->child("camera");
      std::string suffix;
      float variance;

      // Make sure -sg:spp=1
      renderer->child("spp") = 1;

      // Only set denoiser state if denoiser is available
      if (!fb->hasChild("useDenoiser"))
        optDenoiser = 0;

      if (optDenoiser)
        fb->child("useDenoiser") = true;

      do {
        // Clear accum and start fresh
        camera.markAsModified();
        size_t numSamples = 1;

        do {
          // use snprintf to pad number with leading 0s for sorting
          char str[16];
          snprintf(str, sizeof(str), "_%4.4lu", numSamples);
          suffix = str;

          root->renderFrame();
          variance = renderer->getLastVariance();

          // use snprintf to format variance percentage
          snprintf(str, sizeof(str), "_v%2.2f%%", variance);
          suffix += str;

#if 0 // XXX: def OSPRAY_APPS_ENABLE_DENOISER (From AsyncRenderEnginer)
      if (sgFB->auxBuffers()) {
          frameBuffers.back().resize(sgFB->size(), OSP_FB_RGBA32F);
          denoisers.back().map(sgFB, (vec4f*)frameBuffers.back().data());
          denoisers.back().execute();
          denoisers.back().unmap(sgFB);

          suffix += "_D";
      }
#endif

          // Output images for power of 2 samples
          if (optWriteAllImages && (numSamples & (numSamples-1)) == 0)
            writeImage(fb, suffix);

        } while ((numSamples++ < optMaxSamples) && (variance > optMinVariance));

        if (optWriteFinalImage) {
          suffix += "_final";
          writeImage(fb, suffix);
        }

        // Disable denoiser and possibly rerun frame
        if (optDenoiser)
          fb->child("useDenoiser") = false;
      } while (--optDenoiser > 0);
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
          optDenoiser = min(2, max(0, atoi(av[i + 1])));
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
          optMaxSamples = max(0, atoi(av[i + 1]));
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-mv" || arg == "--minvariance") {
          optMinVariance = min(100., max(0., atof(av[i + 1])));
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
