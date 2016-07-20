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

#include "common/commandline/Utility.h"

#ifdef OSPRAY_APPS_ENABLE_SCRIPTING
# include "ScriptedOSPGlutViewer.h"
#else
# include "common/widgets/OSPGlutViewer.h"
#endif

#if OSPRAY_DISPLAY_WALD
#  include "client/Client.h"
#endif

std::string scriptFileFromCommandLine(int ac, const char **&av)
{
  std::string scriptFileName;

  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "--script" || arg == "-s") {
      scriptFileName = av[++i];
    }
  }

  return scriptFileName;
}

void parseForDisplayWall(int ac, const char **&av, ospray::OSPGlutViewer &v)
{
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "--display-wall") {
      ospray::OSPGlutViewer::DisplayWall displayWall;
#if OSPRAY_DISPLAY_WALD
      std::cout << "#osp.glutViewer: using displayWald-style display wall module"
                << std::endl;
      const std::string hostName = av[++i];
      const int portNo = 2903; /* todo: extract that from hostname if required */

      std::cout << "#osp.glutViewer: trying to get display wall config from "
                << hostName << ":" << portNo << std::endl;
      ospray::dw::ServiceInfo service;
      service.getFrom(hostName,portNo);
      const ospcommon::vec2i size = service.totalPixelsInWall;
      std::cout << "#osp.glutViewer: found display wald with " 
                << size.x << "x" << size.y << " pixels "
                << "(" << ospcommon::prettyNumber(size.product()) << "pixels)" 
                << std::endl;
      displayWall.hostname   = service.mpiPortName;
      displayWall.streamName = service.mpiPortName;
      displayWall.size = size;
#else
      displayWall.hostname   = av[++i];
      displayWall.streamName = av[++i];
      displayWall.size.x     = atof(av[++i]);
      displayWall.size.y     = atof(av[++i]);
#endif
      v.setDisplayWall(displayWall);
    }
  }
}

int main(int ac, const char **av)
{
  ospInit(&ac,av);
  ospray::glut3D::initGLUT(&ac,av);

  auto ospObjs = parseWithDefaultParsers(ac, av);

  ospcommon::box3f      bbox;
  ospray::cpp::Model    model;
  ospray::cpp::Renderer renderer;
  ospray::cpp::Camera   camera;

  std::tie(bbox, model, renderer, camera) = ospObjs;

#ifdef OSPRAY_APPS_ENABLE_SCRIPTING
  auto scriptFileName = scriptFileFromCommandLine(ac, av);

  ospray::ScriptedOSPGlutViewer window(bbox, model, renderer,
                                       camera, scriptFileName);
#else
  ospray::OSPGlutViewer window(bbox, model, renderer, camera);
#endif
  window.create("ospGlutViewer: OSPRay Mini-Scene Graph test viewer");

  parseForDisplayWall(ac, av, window);

  ospray::glut3D::runGLUT();
}
