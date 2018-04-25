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

#include "ospcommon/utility/StringManip.h"

#include <string>
#include <vector>

namespace ospray {
  namespace sg {

    struct GatherNodesByName : public Visitor
    {
      GatherNodesByName(const std::string &_name);

      bool operator()(Node &node, TraversalContext &ctx) override;

      std::vector<std::shared_ptr<Node>> results();

    private:
      std::string name;
      std::vector<std::shared_ptr<Node>> nodes;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline GatherNodesByName::GatherNodesByName(const std::string &_name)
        : name(_name)
    {
    }

    inline bool GatherNodesByName::operator()(Node &node, TraversalContext &)
    {
      if (utility::longestBeginningMatch(node.name(), this->name) == this->name) {
        auto itr = std::find_if(
          nodes.begin(),
          nodes.end(),
          [&](const std::shared_ptr<Node> &nodeInList) {
            return nodeInList.get() == &node;
          }
        );

        if (itr == nodes.end())
          nodes.push_back(node.shared_from_this());
      }

      return true;
    }

    inline std::vector<std::shared_ptr<Node>> GatherNodesByName::results()
    {
      return nodes;// TODO: should this be a move (i.e. reader 'consumes')?
    }

  } // ::ospray::sg
} // ::ospray
