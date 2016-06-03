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

#include "sg/common/Node.h"

namespace ospray {
  namespace sg {

    // list of all named nodes - for now use this as a global
    // variable, but eventually we'll need tofind a better way for
    // storing this
    std::map<std::string,Ref<sg::Node> > namedNodes;

    sg::Node *findNamedNode(const std::string &name)
    { 
      if (namedNodes.find(name) != namedNodes.end()) 
        return namedNodes[name].ptr; 
      return NULL; 
    }

    void registerNamedNode(const std::string &name, Ref<sg::Node> node)
    {
      namedNodes[name] = node; 
    }

    void Node::setFromXML(const xml::Node *const node, 
                          const unsigned char *binBasePtr)
    { 
      throw std::runtime_error(toString()+":setFromXML() not implemented for XML node type "
                               +node->name); 
    };

  }
}
