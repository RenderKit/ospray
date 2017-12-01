// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#pragma once
#include "common/sg/SceneGraph.h"

namespace ospray {
namespace app {

class OSPApp {
 public:
  OSPApp();
 protected:
  int initializeOSPRay(int argc, const char *argv[]);

  void parseCommandLine(int ac, const char **&av);
  // parse command line arguments containing the format:
  //  -nodeName:...:nodeName=value,value,value -- changes value
  //  -nodeName:...:nodeName+=name,type        -- adds new child node
  void parseCommandLineSG(int ac, const char **&av, sg::Node &root);

  void addLightsToScene(sg::Node &renderer, bool defaults);
  void addImporterNodesToWorld(sg::Node &renderer);
  void addAnimatedImporterNodesToWorld(sg::Node &renderer);

  struct clTransform {
    vec3f translate{ 0, 0, 0 };
    vec3f scale{ 1, 1, 1 };
    vec3f rotation{ 0, 0, 0 };
  };

  // command line file
  struct clFile {
    clFile(std::string f, const clTransform &t) : file(f), transform(t) {}
    std::string file;
    clTransform transform;
  };

  std::vector<clFile> files;
  std::vector<std::vector<clFile> > animatedFiles;
  std::string initialRendererType;

  std::string hdri_light;
  int matrix_i = 1, matrix_j = 1, matrix_k = 1;
};

} // namespace ospray
} // namespace app
