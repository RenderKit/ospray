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

#include "Node.h"

#include <type_traits>

namespace ospray {
  namespace sg {

    // A list of nodes which will be stored in a particular order, which cannot
    // be guarenteed with Node::children(). Nodes add via push_back() will be
    // parented into the NodeList to propagate change updates from any nodes
    // in the list. Thus using add() will *not* put the given node into the
    // NodeList's list.
    template <typename NODE_T>
    struct NodeList : public Node
    {
      static_assert(std::is_base_of<Node, NODE_T>::value,
                    "NodeList<> can only be instantiated with sg::Node or"
                    " a derived sg::Node type!");

      void clear();
      void push_back(const NODE_T &node);
      void push_back(const std::shared_ptr<NODE_T> &node);

      NODE_T& item(size_t i) const;
      NODE_T& operator[](size_t i) const;

      std::vector<std::shared_ptr<NODE_T>> nodes;
    };

    // Inlined members ////////////////////////////////////////////////////////

    template <typename NODE_T>
    inline void NodeList<NODE_T>::clear()
    {
      nodes.clear();
    }

    template <typename NODE_T>
    inline void NodeList<NODE_T>::push_back(const NODE_T &node)
    {
      nodes.push_back(node.shared_from_this());
      add(node);
    }

    template <typename NODE_T>
    inline void NodeList<NODE_T>::push_back(const std::shared_ptr<NODE_T> &node)
    {
      nodes.push_back(node);
      add(node);
    }

    template <typename NODE_T>
    inline NODE_T& NodeList<NODE_T>::item(size_t i) const
    {
      return *(nodes[i]);
    }

    template <typename NODE_T>
    inline NODE_T& NodeList<NODE_T>::operator[](size_t i) const
    {
      return item(i);
    }

  } // ::ospray::sg
} // ::ospray
