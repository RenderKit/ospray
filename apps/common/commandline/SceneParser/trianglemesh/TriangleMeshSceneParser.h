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

#include <common/commandline/CommandLineExport.h>
#include <common/commandline/SceneParser/SceneParser.h>
#include <ospray/ospray_cpp/Renderer.h>
#include <common/miniSG/miniSG.h>

class OSPRAY_COMMANDLINE_INTERFACE TriangleMeshSceneParser : public SceneParser
{
public:
  TriangleMeshSceneParser(ospray::cpp::Renderer renderer,
                          std::string geometryType = "triangles");

  bool parse(int ac, const char **&av) override;

  std::deque<ospray::cpp::Model> model() const override;
  std::deque<ospcommon::box3f>   bbox()  const override;
  ospray::cpp::Geometry createOSPRayGeometry(ospray::miniSG::Model *msgModel,
                                             ospray::miniSG::Mesh  *msgMesh);

private:

  ospray::cpp::Material createDefaultMaterial(ospray::cpp::Renderer renderer);
  ospray::cpp::Material createMaterial(ospray::cpp::Renderer renderer,
                                       ospray::miniSG::Material *mat);

  ospray::cpp::Renderer renderer;

  std::deque<ospray::cpp::Model> sceneModels;

  std::string geometryType;

  bool alpha;
  bool shouldCreateDefaultMaterial;
  unsigned int maxObjectsToConsider;

  // if turned on, we'll put each triangle mesh into its own instance,
  // no matter what
  bool forceInstancing;
  bool forceNoInstancing;

  ospcommon::Ref<ospray::miniSG::Model> msgModel;
  std::deque<ospcommon::Ref<ospray::miniSG::Model> > msgModels;
  std::vector<ospray::miniSG::Model *> msgAnimation;

  void finalize();
};
