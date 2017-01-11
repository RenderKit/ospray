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

#include "sg/common/Node.h"
#include "sg/common/Data.h"
#include "sg/common/Texture2D.h"

namespace ospray {
  namespace sg {


        // ==================================================================
    // parameter type specializations
    // ==================================================================
    template<> OSPDataType ParamT<std::shared_ptr<DataBuffer> >::getOSPDataType() const
    { return OSP_DATA; }
    template<> OSPDataType ParamT<std::shared_ptr<Node> >::getOSPDataType() const
    { return OSP_OBJECT; }

    template<> OSPDataType ParamT<float>::getOSPDataType() const
    { return OSP_FLOAT; }
    template<> OSPDataType ParamT<vec2f>::getOSPDataType() const
    { return OSP_FLOAT2; }
    template<> OSPDataType ParamT<vec3f>::getOSPDataType() const
    { return OSP_FLOAT3; }
    template<> OSPDataType ParamT<vec4f>::getOSPDataType() const
    { return OSP_FLOAT4; } 

    template<> OSPDataType ParamT<int32_t>::getOSPDataType() const
    { return OSP_INT; }
    template<> OSPDataType ParamT<vec2i>::getOSPDataType() const
    { return OSP_INT2; }
    template<> OSPDataType ParamT<vec3i>::getOSPDataType() const
    { return OSP_INT3; }
    template<> OSPDataType ParamT<vec4i>::getOSPDataType() const
    { return OSP_INT4; }

    template<> OSPDataType ParamT<const char *>::getOSPDataType() const
    { return OSP_STRING; }
    template<> OSPDataType ParamT<std::shared_ptr<Texture2D> >::getOSPDataType() const
    { return OSP_TEXTURE; }

    // ==================================================================
    // sg node implementations
    // ==================================================================
    // void Node::addParam(sg::Param *p)
    // {
    //   assert(p);
    //   assert(p->getName() != "");
    //   param[p->getName()] = p;
    // }    

    void Node::setFromXML(const xml::Node &node,
                          const unsigned char *binBasePtr)
    { 
      throw std::runtime_error(toString()+":setFromXML() not implemented for XML node type "
                               +node.name); 
    };

    // ==================================================================
    // global struff
    // ==================================================================

    
    // list of all named nodes - for now use this as a global
    // variable, but eventually we'll need tofind a better way for
    // storing this
    std::map<std::string,std::shared_ptr<sg::Node> > namedNodes;

    std::shared_ptr<sg::Node> findNamedNode(const std::string &name)
    {
      auto it = namedNodes.find(name);
      if (it != namedNodes.end()) 
        return it->second;                         
      return {};
    }

    void registerNamedNode(const std::string &name, const std::shared_ptr<sg::Node> &node)
    {
      namedNodes[name] = node; 
    }

  }
}
