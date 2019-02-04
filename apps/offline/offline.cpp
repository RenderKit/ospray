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

#if OSPRAY_APPS_ENABLE_DENOISER
#include <OpenImageDenoise/oidn.hpp>
#endif

namespace ospray {
  namespace app {

    struct OSPOffline : public OSPApp
    {
      OSPOffline();

    private:

      void render(const std::shared_ptr<sg::Frame> &) override;
      int parseCommandLine(int &ac, const char **&av) override;
      void printHelp();

      void writeImage(const std::string &);

      std::shared_ptr<sg::FrameBuffer> fb;

#if OSPRAY_APPS_ENABLE_DENOISER
      std::vector<vec4f> denoisedBuffer;
      oidn::DeviceRef denoiserDevice;
      oidn::FilterRef filter;
#endif

      // Default options
      std::string optImageBaseName = "offline";
      bool optWriteAllImages = true;
      bool optWriteAuxBuffers = false;
      bool optWriteFinalImage = true;
      int optDenoiser = 0;
      size_t optMaxSamples = 1024;
      float optMinVariance = 2.f;
    };

    OSPOffline::OSPOffline()
    {
#if OSPRAY_APPS_ENABLE_DENOISER
      denoiserDevice = oidn::newDevice();
      denoiserDevice.commit();
      filter = denoiserDevice.newFilter("RT");
#endif
    }

    void OSPOffline::printHelp()
    {
      std::cout <<
R"text(
./ospOffline [parameters] [scene_files]

ospOffline specific parameters:
   -i     --image [baseFilename] (default 'offline')
            base name of saved images
   -wa    --writeall (default)
   -nwa   --no-writeall
            write (or not) all power of 2 sampled images
   -waux  --writeauxbuffers (default)
   -nwaux --no-writeauxbuffers
   -wf    --writefinal (default)
   -nwf   --no-writefinal
            write (or not) final converged image
   -ms    --maxsamples [int] (default 1024)
            maximum number of samples
   -mv    --minvariance [float] (default 2q)
            minimum variance to which image will converge
)text"
    <<
#ifdef OSPRAY_APPS_ENABLE_DENOISER
R"text(   -oidn  --denoiser [0,1,2] (default 0)
            image denoiser (0 = off, 1 = on, 2 = generate both)
)text"
#else
R"text(
   !!! Denoiser not enabled !!!
)text"
#endif
      << std::endl;
    }

    void OSPOffline::writeImage(const std::string &suffix = "")
    {
      auto imageOutputFile = optImageBaseName + suffix;
      auto fbSize = fb->size();

      if (fb->format() == OSP_FB_RGBA32F) {
        const vec4f *mappedColor = (const vec4f *)fb->map(OSP_FB_COLOR);

#ifdef OSPRAY_APPS_ENABLE_DENOISER
        if (fb->auxBuffers()) {
          const vec3f *mappedNormal = (const vec3f *)fb->map(OSP_FB_NORMAL);
          const vec3f *mappedAlbedo = (const vec3f *)fb->map(OSP_FB_ALBEDO);

          // Write original noisy color buffer and auxilliary buffers
          if (optWriteAuxBuffers) {
            utility::writePFM(imageOutputFile + "_normal.pfm", fbSize.x, fbSize.y, mappedNormal);
            utility::writePFM(imageOutputFile + "_albedo.pfm", fbSize.x, fbSize.y, mappedAlbedo);
            utility::writePFM(imageOutputFile + "_color.pfm", fbSize.x, fbSize.y, mappedColor);
            std::cout << "saved current frame to '" << imageOutputFile << "' color|albeda|normal" << std::endl;
          }

          auto outputBuffer = denoisedBuffer.data();
          bool hdr = !fb->toneMapped();
          filter.set("hdr", hdr);
          filter.setImage("color", (void*)mappedColor, oidn::Format::Float3,
                                   fbSize.x, fbSize.y, 0, sizeof(vec4f));
          filter.setImage("albedo", (void*)mappedAlbedo, oidn::Format::Float3,
                                    fbSize.x, fbSize.y, 0, sizeof(vec3f));
          filter.setImage("normal", (void*)mappedNormal, oidn::Format::Float3,
                                    fbSize.x, fbSize.y, 0, sizeof(vec3f));
          filter.setImage("output", outputBuffer, oidn::Format::Float3,
                                    fbSize.x, fbSize.y, 0, sizeof(vec4f));
          filter.commit();
          filter.execute();

          imageOutputFile += "_D.pfm";
          utility::writePFM(imageOutputFile, fbSize.x, fbSize.y, outputBuffer);
          std::cout << "saved current frame to '" << imageOutputFile << "' (denoised)" << std::endl;

          fb->unmap(mappedNormal);
          fb->unmap(mappedAlbedo);
          fb->unmap(mappedColor);
        } else
#endif
        {
          // ColorBuffer format OSP_FB_RGBA32F, with no denoising auxiliary buffers
          imageOutputFile += ".pfm";
          utility::writePFM(imageOutputFile, fbSize.x, fbSize.y, mappedColor);
          std::cout << "saved current frame to '" << imageOutputFile << "" << std::endl;

          fb->unmap(mappedColor);
        }

      } else {
        const uint32_t *mappedFB = (const uint32_t *)fb->map(OSP_FB_COLOR);
        imageOutputFile += ".ppm";
        utility::writePPM(imageOutputFile, fbSize.x, fbSize.y, mappedFB);
        std::cout << "saved current frame to '" << imageOutputFile << "'" << std::endl;
        fb->unmap(mappedFB);
      }

    }

    void OSPOffline::render(const std::shared_ptr<sg::Frame> &root)
    {
      auto renderer = root->child("renderer").nodeAs<sg::Renderer>();
      fb = root->child("frameBuffer").nodeAs<sg::FrameBuffer>();
      std::string suffix;
      float variance;

      // Allocate denoise buffer the size of framebuffer
      auto fbSize = fb->size();
#if OSPRAY_APPS_ENABLE_DENOISER
      denoisedBuffer.reserve(fbSize.x * fbSize.y);
#endif

      // Setup initial conditions
      // disable use of "navFrameBuffer" for first frame
      // and make sure spp=1
      root->child("navFrameBuffer")["size"] = fb->size();
      renderer->child("spp") = 1;

      // Only set denoiser state if denoiser is available
      if (!fb->hasChild("useDenoiser"))
        optDenoiser = 0;

      if (optDenoiser)
        fb->child("useDenoiser") = true;

      do {
        // Clear accum
        root->child("camera").markAsModified();
        size_t numSamples = 1;

        do {
          // use snprintf to pad number with leading 0s for sorting
          char str[16];
          snprintf(str, sizeof(str), "_%4.4lu", numSamples);
          suffix = str;

          root->renderFrame();
          variance = renderer->getLastVariance();

          // use snprintf to format variance percentage
          snprintf(str, sizeof(str), "_v%2.2fq", variance);
          suffix += str;

          // Output images for power of 2 samples
          if (optWriteAllImages && (numSamples & (numSamples-1)) == 0)
            writeImage(suffix);

        } while ((numSamples++ < optMaxSamples) && (variance > optMinVariance));

        // Output final image (either due to optMaxSamples or optMinVariance)
        if (optWriteFinalImage) {
          suffix += "_final";
          writeImage(suffix);
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
          optImageBaseName = av[i + 1];
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
        } else if (arg == "-waux" || arg == "--writeauxbuffers") {
          optWriteAuxBuffers = true;
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-nwaux" || arg == "--no-writeauxbuffers") {
          optWriteAuxBuffers = false;
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
        } else if (arg == "-ms" || arg == "--maxsamples") {
          optMaxSamples = max(0, atoi(av[i + 1]));
          removeArgs(ac, av, i, 2);
          --i;
        } else if (arg == "-mv" || arg == "--minvariance") {
          optMinVariance = min(100., max(0., atof(av[i + 1])));
          removeArgs(ac, av, i, 2);
          --i;
#if OSPRAY_APPS_ENABLE_DENOISER
        } else if (arg == "-oidn" || arg == "--denoiser") {
          optDenoiser = min(2, max(0, atoi(av[i + 1])));
          removeArgs(ac, av, i, 2);
          --i;
#endif
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
