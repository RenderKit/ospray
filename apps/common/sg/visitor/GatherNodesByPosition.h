// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "ospcommon/utility/StringManip.h"

#include <string>
#include <vector>

namespace ospray {
  namespace sg {

    struct GatherNodesByPosition : public Visitor
    {
      GatherNodesByPosition(const vec3f &);

      bool operator()(Node &node, TraversalContext &ctx) override;

      std::vector<std::shared_ptr<Node>> results();

    private:
      vec3f pos;
      std::vector<std::shared_ptr<Node>> nodes;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline GatherNodesByPosition::GatherNodesByPosition(const vec3f &_pos)
        : pos(_pos)
    {
    }

    inline bool GatherNodesByPosition::operator()(Node &node, TraversalContext &)
    {
      if (dynamic_cast<sg::Geometry*>(&node) && node.bounds().contains(pos))
        nodes.push_back(node.shared_from_this());

      return true;
    }

    inline std::vector<std::shared_ptr<Node>> GatherNodesByPosition::results()
    {
      return nodes;// TODO: should this be a move (i.e. reader 'consumes')?
    }

  } // ::ospray::sg
} // ::ospray
