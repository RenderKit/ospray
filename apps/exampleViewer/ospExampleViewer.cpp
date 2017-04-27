// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include <ospray/ospray_cpp/Device.h>
#include <ospray/ospray_cpp/FrameBuffer.h>
#include <ospray/ospray_cpp/Renderer.h>
#include "common/commandline/Utility.h"
#include "ospcommon/Socket.h"

#include "widgets/imguiViewer.h"

namespace exampleViewer {

  using namespace commandline;

  ospcommon::vec3f translate;
  ospcommon::vec3f scale;
  bool lockFirstFrame = false;
  bool fullscreen = false;
  std::string displayWall = "";

  namespace dw {
    
    struct ServiceInfo {
      /* constructor that initializes everything to default values */
      ServiceInfo()
        : totalPixelsInWall(-1,-1),
          mpiPortName("<value not set>")
      {}
      
      /*! total pixels in the entire display wall, across all
        indvididual displays, and including bezels (future versios
        will allow to render to smaller resolutions, too - and have
        the clients upscale this - but for now the client(s) have to
        render at exactly this resolution */
      ospcommon::vec2i totalPixelsInWall;

      /*! the MPI port name that the service is listening on client
        connections for (ie, the one to use with
        client::establishConnection) */
      std::string mpiPortName; 

      /*! whether this runs in stereo mode */
      int stereo;

      /*! read a service info from a given hostName:port. The service
        has to already be running on that port 

        Note this may throw a std::runtime_error if the connection
        cannot be established 
      */
      void getFrom(const std::string &hostName,
                   const int portNo);
    };
    /*! read a service info from a given hostName:port. The service
      has to already be running on that port */
    void ServiceInfo::getFrom(const std::string &hostName,
                              const int portNo)
    {
      ospcommon::socket_t sock = ospcommon::connect(hostName.c_str(),portNo);
      if (!sock)
        throw std::runtime_error("could not create display wall connection!");

      mpiPortName = read_string(sock);
      totalPixelsInWall.x = read_int(sock);
      totalPixelsInWall.y = read_int(sock);
      stereo = read_int(sock);
      close(sock);
    }
  }

  void parseExtraParametersFromComandLine(int ac, const char **&av)
  {
    for (int i = 1; i < ac; i++) {
      const std::string arg = av[i];
      if (arg == "--translate") {
        translate.x = atof(av[++i]);
        translate.y = atof(av[++i]);
        translate.z = atof(av[++i]);
      } else if (arg == "--display-wall" || arg == "-dw") {
        displayWall = av[++i];
      } else if (arg == "--scale") {
        scale.x = atof(av[++i]);
        scale.y = atof(av[++i]);
        scale.z = atof(av[++i]);
      } else if (arg == "--lockFirstFrame") {
        lockFirstFrame = true;
      } else if (arg == "--fullscreen") {
        fullscreen = true;
      }
    }
  }

  extern "C" int main(int ac, const char **av)
  {
    ospInit(&ac,av);

    ospray::imgui3D::init(&ac,av);

    auto ospObjs = parseWithDefaultParsersDW(ac, av);

    std::deque<ospcommon::box3f>   bbox;
    std::deque<ospray::cpp::Model> model;
    ospray::cpp::Renderer renderer;
    ospray::cpp::Renderer rendererDW;
    ospray::cpp::Camera   camera;
    ospray::cpp::FrameBuffer frameBufferDW;
    
    std::tie(bbox, model, renderer, rendererDW, camera) = ospObjs;
    
    parseExtraParametersFromComandLine(ac, av);
    
    if (displayWall != "") {
      std::cout << "#############################################" << std::endl;
      std::cout << "found --display-wall cmdline argument ...." << std::endl;
      std::cout << "trying to connect to display wall service on "
                << displayWall << ":2903" << std::endl;
      
      dw::ServiceInfo dwService;
      dwService.getFrom(displayWall,2903);
      std::cout << "found display wall service on MPI port "
                << dwService.mpiPortName << std::endl;
      std::cout << "#############################################" << std::endl;
      frameBufferDW = ospray::cpp::FrameBuffer(dwService.totalPixelsInWall,
                                               (OSPFrameBufferFormat)OSP_FB_NONE,
                                               OSP_FB_COLOR|OSP_FB_ACCUM);
      
      ospLoadModule("displayWald");
      OSPPixelOp pixelOp = ospNewPixelOp("display_wald");
      ospSetString(pixelOp,"streamName",dwService.mpiPortName.c_str());
      ospCommit(pixelOp);
      ospSetPixelOp(frameBufferDW.handle(),pixelOp);
      rendererDW.set("frameBuffer", frameBufferDW.handle());
      rendererDW.commit();
    } else {
      // no diplay wall - nix the display wall renderer
      rendererDW = ospray::cpp::Renderer();
    }

    ospray::ImGuiViewer window(bbox, model, renderer, rendererDW,
                               frameBufferDW, camera);
    window.setScale(scale);
    window.setLockFirstAnimationFrame(lockFirstFrame);
    window.setTranslation(translate);
    window.create("ospImGui: OSPRay ImGui Viewer App", fullscreen);

    ospray::imgui3D::run();
    return 0;
  }

}
