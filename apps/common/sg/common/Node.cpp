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
#include "sg/common/RenderContext.h"

namespace ospray {
  namespace sg {

    std::vector<std::shared_ptr<sg::Node>> Node::nodes;

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

    // NOTE(jda) - can't do default member initializers due to MSVC...
    Node::Node()
    {
      properties.name = "NULL";
      properties.type = "Node";
      markAsModified();
    }

    std::string Node::toString() const
    {
      return "ospray::sg::Node";
    }

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

    void Node::init()
    {
    }

    void Node::render(RenderContext &ctx)
    {
    }

    void Node::commit()
    {
    }

    std::string Node::documentation()
    {
      return properties.documentation;
    }

    void Node::setDocumentation(const std::string &s)
    {
      properties.documentation = s;
    }

    box3f Node::bounds() const
    {
      return empty;
    }

    TimeStamp Node::lastModified() const
    {
      return properties.lastModified;
    }

    TimeStamp Node::childrenLastModified() const
    {
      return properties.childrenMTime;
    }

    TimeStamp Node::lastCommitted() const
    {
      return properties.lastCommitted;
    }

    void Node::markAsCommitted()
    {
      properties.lastCommitted = TimeStamp();
    }

    void Node::markAsModified()
    {
      properties.lastModified = TimeStamp();
      if (!parent().isNULL())
        parent()->setChildrenModified(properties.lastModified);
    }

    void Node::setChildrenModified(TimeStamp t)
    {
      if (t > properties.childrenMTime) {
        properties.childrenMTime = t;
        if (!parent().isNULL())
          parent()->setChildrenModified(properties.childrenMTime);
      }
    }

    bool Node::hasChild(const std::string &name) const
    {
      std::lock_guard<std::mutex> lock{mutex};
      auto itr = properties.children.find(name);
      return itr != properties.children.end();
    }

    Node::Handle Node::child(const std::string &name) const
    {
      std::lock_guard<std::mutex> lock{mutex};
      auto itr = properties.children.find(name);
      if (itr == properties.children.end()) {
        throw std::runtime_error("in node "+toString()+
                                 " : could not find sg child node with name '"+name+"'");
      } else {
        return itr->second;
      }
    }

    Node::Handle Node::childRecursive(const std::string &name)
    {
      mutex.lock();
      Node* n = this;
      auto f = n->properties.children.find(name);
      if (f != n->properties.children.end()) {
        mutex.unlock();
        return f->second;
      }

      for (auto &child : properties.children) {
        mutex.unlock();
        Handle r = child.second->childRecursive(name);
        if (!r.isNULL())
          return r;
        mutex.lock();
      }

      mutex.unlock();
      return Handle();
    }

    std::vector<Node::Handle> Node::childrenByType(const std::string &t) const
    {
      std::lock_guard<std::mutex> lock{mutex};
      std::vector<Handle> result;
      NOT_IMPLEMENTED;
      return result;
    }

    std::vector<Node::Handle> Node::children() const
    {
      std::lock_guard<std::mutex> lock{mutex};
      std::vector<Handle> result;
      for (auto &child : properties.children)
        result.push_back(child.second);
      return result;
    }

    Node::Handle Node::operator[](const std::string &c) const
    {
      return child(c);
    }

    Node::Handle Node::parent() const 
    {
      return properties.parent;
    }

    void Node::setParent(const Node::Handle &p)
    {
      std::lock_guard<std::mutex> lock{mutex};
      properties.parent = p;
    }

    SGVar Node::value()
    {
      std::lock_guard<std::mutex> lock{mutex};
      return properties.value;
    }

    void Node::setValue(SGVar val)
    {
      {
        std::lock_guard<std::mutex> lock{mutex};
        if (val != properties.value)
          properties.value = val;
      }

      markAsModified();
    }

    void Node::add(std::shared_ptr<Node> node)
    {
      std::lock_guard<std::mutex> lock{mutex};
      properties.children[node->name()] = Handle(node);

      //ARG!  Cannot call shared_from_this in constructors.  PIA!!!
      node->setParent(shared_from_this());
    }

    void Node::add(Node::Handle node)
    {
      std::lock_guard<std::mutex> lock{mutex};
      properties.children[node->name()] = node;
      node->setParent(shared_from_this());
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

    void Node::traverse(const std::string &operation)
    {
      RenderContext ctx;
      traverse(ctx, operation);
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

    void Node::preCommit(RenderContext &ctx)
    {
    }

    void Node::postCommit(RenderContext &ctx)
    {
    }

    void Node::setName(const std::string &v)
    {
      properties.name = v;
    }

    void Node::setType(const std::string &v)
    {
      properties.type = v;
    }

    std::string Node::name() const
    {
      return properties.name;
    }

    std::string Node::type() const
    {
      return properties.type;
    }

    void Node::setFlags(NodeFlags f)
    {
      properties.flags = f;
    }

    void Node::setFlags(int f)
    {
      setFlags(static_cast<NodeFlags>(f));
    }

    NodeFlags Node::flags() const
    {
      return properties.flags;
    }

    void Node::setMinMax(const SGVar &minv, const SGVar &maxv)
    {
      properties.minmax.resize(2);
      properties.minmax[0] = minv;
      properties.minmax[1] = maxv;
    }

    SGVar Node::min() const
    {
      return properties.minmax[0];
    }

    SGVar Node::max() const
    {
      return properties.minmax[1];
    }

    void Node::setWhiteList(const std::vector<SGVar> &values)
    {
      properties.whitelist = values;
    }

    void Node::setBlackList(const std::vector<SGVar> &values)
    {
      properties.blacklist = values;
    }

    bool Node::isValid()
    {
      return properties.valid;
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

    bool Node::computeValidMinMax()
    {
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

    NodeHandle createNode(std::string name, std::string type, SGVar var,
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

      if (var.valid()) newNode->setValue(var);

      return newNode;
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
