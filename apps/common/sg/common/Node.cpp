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

    // ==================================================================
    // parameter type specializations
    // ==================================================================

    template<> OSPDataType ParamT<std::shared_ptr<DataBuffer>>::getOSPDataType() const
    { return OSP_DATA; }
    template<> OSPDataType ParamT<std::shared_ptr<Node>>::getOSPDataType() const
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
    template<> OSPDataType ParamT<std::shared_ptr<Texture2D>>::getOSPDataType() const
    { return OSP_TEXTURE; }

    // ==================================================================
    // sg node implementations
    // ==================================================================

    std::shared_ptr<sg::Param> Node::getParam(const std::string &name) const
    {
      auto it = params.find(name);

      if (it != params.end())
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

      ctx.childMTime = TimeStamp();
      preTraverse(ctx, operation);
      ctx.level++;

      for (auto child : children)
        child.second->traverse(ctx, operation);

      ctx.level--;
      ctx.childMTime = getChildrenLastModified();
      postTraverse(ctx, operation);
    }

    void Node::preTraverse(RenderContext &ctx, const std::string& operation)
    {
      if (operation == "print") {
        for (int i=0;i<ctx.level;i++)
          std::cout << "  ";
        std::cout << name() << " : " << type() << "\n";
      } else if (operation == "commit" &&
               (getLastModified() >= getLastCommitted() ||
                getChildrenLastModified() >= getLastCommitted())) {
        preCommit(ctx);
      } else if (operation == "verify") {
        valid = computeValid();
      } else if (operation == "modified") {
        modified();
      }
    }

    void Node::postTraverse(RenderContext &ctx, const std::string& operation)
    {
      if (operation == "commit" &&
          (getLastModified() >= getLastCommitted() ||
           getChildrenLastModified() >= getLastCommitted()))
      {
        postCommit(ctx);
        lastCommitted = TimeStamp();
      }
      else if (operation == "verify")
      {
        for (auto child : children)
        {
          if (child.second->getFlags() & NodeFlags::required)
            valid &= child.second->isValid();
        }
      }
    }

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
