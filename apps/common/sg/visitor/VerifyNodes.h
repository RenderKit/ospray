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

    //Visitor node used to set nodes to valid
    struct VerifyNodes : public Visitor
    {
      VerifyNodes(bool errorOut = true) : errorOnInvalid(errorOut) {}

      bool operator()(Node &node, TraversalContext &) override;
      void postChildren(Node &node, TraversalContext &) override;

    private:

      bool errorOnInvalid = true;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline bool VerifyNodes::operator()(Node &node, TraversalContext &)
    {
      bool traverseChildren =
          !(node.properties.valid &&
          (node.childrenLastModified() < node.properties.lastVerified));
      node.properties.valid = node.computeValid();
      node.properties.lastVerified = TimeStamp();

      return traverseChildren;
    }

    inline void VerifyNodes::postChildren(Node &node, TraversalContext &)
    {
      for (const auto &child : node.properties.children) {
        if (child.second->flags() & NodeFlags::required)
          node.properties.valid &= child.second->isValid();
      }
      if (errorOnInvalid && !node.properties.valid)
        throw std::runtime_error(node.name() + " was marked invalid");
    }

  } // ::ospray::sg
} // ::ospray

