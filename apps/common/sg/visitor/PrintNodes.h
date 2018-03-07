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

#include "Visitor.h"
#include "../common/Node.h"

namespace ospray {
  namespace sg {

    struct PrintNodes : public Visitor
    {
      PrintNodes() = default;

      bool operator()(Node &node, TraversalContext &ctx) override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline bool PrintNodes::operator()(Node &node, TraversalContext &ctx)
    {
      for (int i = 0; i < ctx.level; i++)
        std::cout << "  ";

      auto name = node.name();
      auto type = node.type();

      std::cout << name << " : " << type << "=\"";

      if (type == "string")
        std::cout << node.valueAs<std::string>();
      if (type == "float")
        std::cout << node.valueAs<float>();
      if (type == "vec3f")
        std::cout << node.valueAs<vec3f>();
      if (type == "vec2i")
        std::cout << node.valueAs<vec2i>();

      std::cout << "\"\n";

      return true;
    }

  } // ::ospray::sg
} // ::ospray
