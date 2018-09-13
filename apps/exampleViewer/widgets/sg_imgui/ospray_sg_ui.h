// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include <memory>
#include <string>

#include "sg/common/Node.h"

#include "imgui.h"

namespace ospray {

  void guiSGSingleNode(const std::string &baseText,
                       std::shared_ptr<sg::Node> node);

  void guiSGTree(const std::string &name, std::shared_ptr<sg::Node> node);

  void guiNodeContextMenu(const std::string &name,
                          std::shared_ptr<sg::Node> node);

  static const ImGuiWindowFlags g_defaultWindowFlags {
    ImGuiWindowFlags_ShowBorders |
    ImGuiWindowFlags_NoCollapse
  };

} // namespace ospray
