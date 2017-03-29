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

#include <common/commandline/SceneParser/SceneParser.h>
#include <ospray/ospray_cpp/Renderer.h>

class TachyonSceneParser : public SceneParser
{
public:
  TachyonSceneParser(ospray::cpp::Renderer);

  bool parse(int ac, const char **&av) override;

  std::deque<ospray::cpp::Model> model() const override;
  std::deque<ospcommon::box3f>   bbox()  const override;

private:

  ospray::cpp::Renderer renderer;
  std::deque<ospray::cpp::Model>    sceneModels;
  std::deque<ospcommon::box3f>      sceneBboxs;
};
