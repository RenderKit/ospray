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

ospcommon::vec3f translate;
ospcommon::vec3f scale;
bool lockFirstFrame = false;

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

int main(int ac, const char **av)
{
#if 0
  ospInit(&ac,av);
#else
  auto device = ospCreateDevice();
  //auto device = ospCreateDevice("mpi");

  // set device parameters...
  //ospDeviceSet1i(device, "numThreads", 1);
  //ospDeviceSet1i(device, "logLevel", 2);
  //ospDeviceSet1i(device, "debug", 1);
  ospDeviceCommit(device);

  ospSetCurrentDevice(device);
#endif

  ospray::glut3D::initGLUT(&ac,av);

  auto ospObjs = parseWithDefaultParsers(ac, av);

  std::deque<ospcommon::box3f>   bbox;
  std::deque<ospray::cpp::Model> model;
  ospray::cpp::Renderer renderer;
  ospray::cpp::Camera   camera;

  std::tie(bbox, model, renderer, camera) = ospObjs;

  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "--translate") {
      translate.x = atof(av[++i]);
      translate.y = atof(av[++i]);
      translate.z = atof(av[++i]);
    } else if (arg == "--scale") {
      scale.x = atof(av[++i]);
      scale.y = atof(av[++i]);
      scale.z = atof(av[++i]);
    } else if (arg == "--lockFirstFrame") {
      lockFirstFrame = true;
    }
  }

#ifdef OSPRAY_APPS_ENABLE_SCRIPTING
  auto scriptFileName = scriptFileFromCommandLine(ac, av);

  ospray::ScriptedOSPGlutViewer window(bbox, model, renderer,
                                       camera, scriptFileName);
#else
  ospray::OSPGlutViewer window(bbox, model, renderer, camera);
#endif
  window.setScale(scale);
  window.setLockFirstAnimationFrame(lockFirstFrame);
  window.setTranslation(translate);
  window.create("ospGlutViewer: OSPRay Mini-Scene Graph test viewer");

  ospray::glut3D::runGLUT();
}
