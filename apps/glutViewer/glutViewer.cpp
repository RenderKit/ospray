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

#include "ScriptedOSPGlutViewer.h"

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
  ospInit(&ac,av);
  ospray::glut3D::initGLUT(&ac,av);

  auto ospObjs = parseWithDefaultParsers(ac, av);

  ospcommon::box3f      bbox;
  ospray::cpp::Model    model;
  ospray::cpp::Renderer renderer;
  ospray::cpp::Camera   camera;

  std::tie(bbox, model, renderer, camera) = ospObjs;

  auto scriptFileName = scriptFileFromCommandLine(ac, av);

  ospray::ScriptedOSPGlutViewer window(bbox, model, renderer,
                                       camera, scriptFileName);
  window.create("ospDebugViewer: OSPRay Mini-Scene Graph test viewer");

  ospray::glut3D::runGLUT();
}
