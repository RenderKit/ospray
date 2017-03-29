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
#include <ospray/ospray_cpp/TransferFunction.h>
#include <common/miniSG/miniSG.h>

#include <limits>
#include <string>
#include <vector>

namespace commandline {

  class OSPRAY_COMMANDLINE_INTERFACE VolumeSceneParser : public SceneParser
  {
  public:
    VolumeSceneParser(ospray::cpp::Renderer);

    bool parse(int ac, const char **&av) override;

    std::deque<ospray::cpp::Model> model() const override;
    std::deque<ospcommon::box3f>   bbox()  const override;

  private:

    // Helper functions //

    void importObjectsFromFile(const std::string &filename,
                               bool loadedTransferFunction);
    void importTransferFunction(const std::string &filename);
    void createDefaultTransferFunction();

    // Data //

    ospray::cpp::Renderer renderer;
    ospcommon::box3f      sceneBbox;

    std::unique_ptr<ospray::cpp::Model> sceneModel;

    float samplingRate{0.125f};

    float tf_scale{1.f};
    std::vector<ospcommon::vec4f> tf_colors;
    std::vector<float> isosurfaces;

    ospray::cpp::TransferFunction transferFunction;
  };

} // ::commandline
