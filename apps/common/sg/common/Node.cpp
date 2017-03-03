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

    std::vector<std::shared_ptr<sg::Node>> Node::nodes;

    /*! return the ospray data type for a given string-ified type */
    OSPDataType getOSPDataTypeFor(const char *string) 
    {
      if (string == NULL)                return(OSP_UNKNOWN);
      if (strcmp(string, "char"  ) == 0) return(OSP_CHAR);
      if (strcmp(string, "double") == 0) return(OSP_DOUBLE);
      if (strcmp(string, "float" ) == 0) return(OSP_FLOAT);
      if (strcmp(string, "float2") == 0) return(OSP_FLOAT2);
      if (strcmp(string, "float3") == 0) return(OSP_FLOAT3);
      if (strcmp(string, "float4") == 0) return(OSP_FLOAT4);
      if (strcmp(string, "int"   ) == 0) return(OSP_INT);
      if (strcmp(string, "int2"  ) == 0) return(OSP_INT2);
      if (strcmp(string, "int3"  ) == 0) return(OSP_INT3);
      if (strcmp(string, "int4"  ) == 0) return(OSP_INT4);
      if (strcmp(string, "uchar" ) == 0) return(OSP_UCHAR);
      if (strcmp(string, "uchar2") == 0) return(OSP_UCHAR2);
      if (strcmp(string, "uchar3") == 0) return(OSP_UCHAR3);
      if (strcmp(string, "uchar4") == 0) return(OSP_UCHAR4);
      if (strcmp(string, "short" ) == 0) return(OSP_SHORT);
      if (strcmp(string, "ushort") == 0) return(OSP_USHORT);
      if (strcmp(string, "uint"  ) == 0) return(OSP_UINT);
      if (strcmp(string, "uint2" ) == 0) return(OSP_UINT2);
      if (strcmp(string, "uint3" ) == 0) return(OSP_UINT3);
      if (strcmp(string, "uint4" ) == 0) return(OSP_UINT4);
      return(OSP_UNKNOWN);
    }

    /*! return the ospray data type for a given string-ified type */
    OSPDataType getOSPDataTypeFor(const std::string &typeName)
    { return getOSPDataTypeFor(typeName.c_str()); }


    /*! return the size (in byte) for a given ospray data type */
    size_t sizeOf(const OSPDataType type) 
    {
      switch (type) {
      case OSP_VOID_PTR:
      case OSP_OBJECT:
      case OSP_CAMERA:
      case OSP_DATA:
      case OSP_DEVICE:
      case OSP_FRAMEBUFFER:
      case OSP_GEOMETRY:
      case OSP_LIGHT:
      case OSP_MATERIAL:
      case OSP_MODEL:
      case OSP_RENDERER:
      case OSP_TEXTURE:
      case OSP_TRANSFER_FUNCTION:
      case OSP_VOLUME:
      case OSP_PIXEL_OP:
      case OSP_STRING:    return sizeof(void *);
      case OSP_CHAR:      return sizeof(int8_t);
      case OSP_UCHAR:     return sizeof(uint8_t);
      case OSP_UCHAR2:    return sizeof(vec2uc);
      case OSP_UCHAR3:    return sizeof(vec3uc);
      case OSP_UCHAR4:    return sizeof(vec4uc);
      case OSP_SHORT:     return sizeof(int16_t);
      case OSP_USHORT:    return sizeof(uint16_t);
      case OSP_INT:       return sizeof(int32_t);
      case OSP_INT2:      return sizeof(vec2i);
      case OSP_INT3:      return sizeof(vec3i);
      case OSP_INT4:      return sizeof(vec4i);
      case OSP_UINT:      return sizeof(uint32_t);
      case OSP_UINT2:     return sizeof(vec2ui);
      case OSP_UINT3:     return sizeof(vec3ui);
      case OSP_UINT4:     return sizeof(vec4ui);
      case OSP_LONG:      return sizeof(int64_t);
      case OSP_LONG2:     return sizeof(vec2l);
      case OSP_LONG3:     return sizeof(vec3l);
      case OSP_LONG4:     return sizeof(vec4l);
      case OSP_ULONG:     return sizeof(uint64_t);
      case OSP_ULONG2:    return sizeof(vec2ul);
      case OSP_ULONG3:    return sizeof(vec3ul);
      case OSP_ULONG4:    return sizeof(vec4ul);
      case OSP_FLOAT:     return sizeof(float);
      case OSP_FLOAT2:    return sizeof(vec2f);
      case OSP_FLOAT3:    return sizeof(vec3f);
      case OSP_FLOAT4:    return sizeof(vec4f);
      case OSP_FLOAT3A:   return sizeof(vec3fa);
      case OSP_DOUBLE:    return sizeof(double);
      case OSP_UNKNOWN:   break;
      };

      std::stringstream error;
      error << __FILE__ << ":" << __LINE__ << ": unknown OSPDataType "
            << (int)type;
      throw std::runtime_error(error.str());
    }

    // ==================================================================
    // parameter type specializations
    // ==================================================================

    template<> OSPDataType ParamT<std::shared_ptr<DataBuffer>>::OSPType() const
    { return OSP_DATA; }
    template<> OSPDataType ParamT<std::shared_ptr<Node>>::OSPType() const
    { return OSP_OBJECT; }

    template<> OSPDataType ParamT<float>::OSPType() const
    { return OSP_FLOAT; }
    template<> OSPDataType ParamT<vec2f>::OSPType() const
    { return OSP_FLOAT2; }
    template<> OSPDataType ParamT<vec3f>::OSPType() const
    { return OSP_FLOAT3; }
    template<> OSPDataType ParamT<vec4f>::OSPType() const
    { return OSP_FLOAT4; }

    template<> OSPDataType ParamT<int32_t>::OSPType() const
    { return OSP_INT; }
    template<> OSPDataType ParamT<vec2i>::OSPType() const
    { return OSP_INT2; }
    template<> OSPDataType ParamT<vec3i>::OSPType() const
    { return OSP_INT3; }
    template<> OSPDataType ParamT<vec4i>::OSPType() const
    { return OSP_INT4; }

    template<> OSPDataType ParamT<const char *>::OSPType() const
    { return OSP_STRING; }
    template<> OSPDataType ParamT<std::shared_ptr<Texture2D>>::OSPType() const
    { return OSP_TEXTURE; }

    // ==================================================================
    // sg node implementations
    // ==================================================================

    std::shared_ptr<sg::Param> Node::param(const std::string &name) const
    {
      auto it = properties.params.find(name);

      if (it != properties.params.end())
        return it->second;

      return {};
    }

    void Node::setFromXML(const xml::Node &node, const unsigned char *binBasePtr)
    {
      throw std::runtime_error(toString() +
                               ":setFromXML() not implemented for XML node type "
                               + node.name);
    }

    void Node::traverse(RenderContext &ctx, const std::string& operation)
    {
      //TODO: make child m time propagate
      if (operation != "verify" && !isValid())
        return;

      ctx._childMTime = TimeStamp();
      preTraverse(ctx, operation);
      ctx.level++;

      for (auto &child : properties.children)
        child.second->traverse(ctx, operation);

      ctx.level--;
      ctx._childMTime = childrenLastModified();
      postTraverse(ctx, operation);
    }

    void Node::preTraverse(RenderContext &ctx, const std::string& operation)
    {
      if (operation == "print") {
        for (int i=0;i<ctx.level;i++)
          std::cout << "  ";
        std::cout << name() << " : " << type() << "\n";
      } else if (operation == "commit" &&
               (lastModified() >= lastCommitted() ||
                childrenLastModified() >= lastCommitted())) {
        preCommit(ctx);
      } else if (operation == "verify") {
        properties.valid = computeValid();
      } else if (operation == "modified") {
        markAsModified();
      }
    }

    void Node::postTraverse(RenderContext &ctx, const std::string& operation)
    {
      if (operation == "commit" &&
          (lastModified() >= lastCommitted() ||
           childrenLastModified() >= lastCommitted())) {
        postCommit(ctx);
        markAsCommitted();
      } else if (operation == "verify") {
        for (const auto &child : properties.children) {
          if (child.second->flags() & NodeFlags::required)
            properties.valid &= child.second->isValid();
        }
      }
    }

    bool Node::computeValid()
    {
#ifndef _WIN32
# warning "Are validation node flags mutually exclusive?"
#endif

      if ((flags() & NodeFlags::valid_min_max) &&
          properties.minmax.size() > 1) {
        if (!computeValidMinMax())
          return false;
      }

      if (flags() & NodeFlags::valid_blacklist) {
        return std::find(properties.blacklist.begin(),
                         properties.blacklist.end(),
                         value()) == properties.blacklist.end();
      }

      if (flags() & NodeFlags::valid_whitelist) {
        return std::find(properties.whitelist.begin(),
                         properties.whitelist.end(),
                         value()) != properties.whitelist.end();
      }

      return true;
    }


    // ==================================================================
    // Renderable
    // ==================================================================

    void Renderable::preTraverse(RenderContext &ctx,
                                 const std::string& operation)
    {
      Node::preTraverse(ctx,operation);
      if (operation == "render")
        preRender(ctx);
    }

    void Renderable::postTraverse(RenderContext &ctx,
                                  const std::string& operation)
    {
      Node::postTraverse(ctx,operation);
      if (operation == "render")
        postRender(ctx);
    }

    // ==================================================================
    // global stuff
    // ==================================================================

    bool valid(SGVar var) { return var.valid(); }

    // list of all named nodes - for now use this as a global
    // variable, but eventually we'll need tofind a better way for
    // storing this
    std::map<std::string, std::shared_ptr<sg::Node>> namedNodes;

    std::shared_ptr<sg::Node> findNamedNode(const std::string &name)
    {
      auto it = namedNodes.find(name);
      if (it != namedNodes.end())
        return it->second;
      return {};
    }

    void registerNamedNode(const std::string &name,
                           const std::shared_ptr<sg::Node> &node)
    {
      namedNodes[name] = node;
    }

    using CreatorFct = std::shared_ptr<sg::Node>(*)();

    std::map<std::string, CreatorFct> nodeRegistry;

    Node::NodeH createNode(std::string name, std::string type, SGVar var,
                           int flags, std::string documentation)
    {
      std::map<std::string, CreatorFct>::iterator it = nodeRegistry.find(type);
      CreatorFct creator = nullptr;
      if (it == nodeRegistry.end()) {
        std::string creatorName = "ospray_create_sg_node__"+std::string(type);
        creator = (CreatorFct)getSymbol(creatorName);
        if (!creator)
          throw std::runtime_error("unknown ospray scene graph node '"+type+"'");
        else {
          std::cout << "#osp:sg: creating at least one instance of node type '"
                    << type << "'" << std::endl;
        }
        nodeRegistry[type] = creator;
      } else {
        creator = it->second;
      }

      std::shared_ptr<sg::Node> newNode = creator();
      Node::nodes.push_back(newNode);
      newNode->init();
      newNode->setName(name);
      newNode->setType(type);
      newNode->setFlags(flags);
      newNode->setDocumentation(documentation);
      if (valid(var)) newNode->setValue(var);
      NodeH result = Node::NodeH(newNode);
      return result;
    }

    OSP_REGISTER_SG_NODE(Node);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<vec3f>, vec3f);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<vec2f>, vec2f);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<vec2i>, vec2i);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<float>, float);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<int>, int);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<bool>, bool);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<std::string>, string);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<box3f>, box3f);
    OSP_REGISTER_SG_NODE_NAME(NodeParam<OSPObject>, OSPObject);

  } // ::ospray::sg
} // ::ospray
