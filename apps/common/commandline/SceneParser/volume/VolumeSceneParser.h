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

#pragma once

#include <common/commandline/SceneParser/SceneParser.h>
#include <ospray_cpp/Renderer.h>
#include <ospray_cpp/TransferFunction.h>
#include <common/miniSG/miniSG.h>

#include <limits>
#include <string>
#include <vector>

class VolumeSceneParser : public SceneParser
{
public:
  VolumeSceneParser(ospray::cpp::Renderer);

  bool parse(int ac, const char **&av) override;

  ospray::cpp::Model model() const override;
  ospcommon::box3f   bbox()  const override;

private:

  // Helper functions //

  void importObjectsFromFile(const std::string &filename);
  void createDefaultTransferFunction();

  // Data //

  ospray::cpp::Renderer m_renderer;
  ospray::cpp::Model    m_model;
  ospcommon::box3f      m_bbox;

  float m_samplingRate{0.125f};

  float m_tf_scale{1.f};
  std::vector<ospcommon::vec4f> m_tf_colors;
  std::vector<float> m_isosurfaces;

  ospray::cpp::TransferFunction m_tf{"piecewise_linear"};
};
