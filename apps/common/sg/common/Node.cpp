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

    // ==================================================================
    // sg node implementations
    // ==================================================================

    // NOTE(jda) - can't do default member initializers due to MSVC...
    Node::Node()
    {
      properties.name = "NULL";
      properties.type = "Node";
      // MSVC 2013 is buggy and ignores {}-initialization of anonymous structs
#if _MSC_VER <= 1800
      properties.parent = nullptr;
      properties.valid = false;
#endif
      markAsModified();
    }

    std::string Node::toString() const
    {
      return "ospray::sg::Node";
    }

    void Node::setFromXML(const xml::Node &node, const unsigned char *binBasePtr)
    {
      throw std::runtime_error(toString() +
                               ":setFromXML() not implemented for XML node type "
                               + node.name);
    }

    void Node::serialize(sg::Serialization::State &state)
    {
    }

    // Properties /////////////////////////////////////////////////////////////

    std::string Node::name() const
    {
      return properties.name;
    }

    std::string Node::type() const
    {
      return properties.type;
    }

    SGVar Node::min() const
    {
      return properties.minmax[0];
    }

    SGVar Node::max() const
    {
      return properties.minmax[1];
    }

    NodeFlags Node::flags() const
    {
      return properties.flags;
    }

    std::string Node::documentation() const
    {
      return properties.documentation;
    }

    void Node::setName(const std::string &v)
    {
      properties.name = v;
    }

    void Node::setType(const std::string &v)
    {
      properties.type = v;
    }

    void Node::setMinMax(const SGVar &minv, const SGVar &maxv)
    {
      properties.minmax.resize(2);
      properties.minmax[0] = minv;
      properties.minmax[1] = maxv;
    }

    void Node::setFlags(NodeFlags f)
    {
      properties.flags = f;
    }

    void Node::setFlags(int f)
    {
      setFlags(static_cast<NodeFlags>(f));
    }

    void Node::setDocumentation(const std::string &s)
    {
      properties.documentation = s;
    }

    void Node::setWhiteList(const std::vector<SGVar> &values)
    {
      properties.whitelist = values;
    }

    void Node::setBlackList(const std::vector<SGVar> &values)
    {
      properties.blacklist = values;
    }

    bool Node::isValid() const
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

    box3f Node::bounds() const
    {
      return empty;
    }

    // Node stored value (data) interface /////////////////////////////////////

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

    // Update detection interface /////////////////////////////////////////////

    TimeStamp Node::lastModified() const
    {
      return properties.lastModified;
    }

    TimeStamp Node::lastCommitted() const
    {
      return properties.lastCommitted;
    }

    TimeStamp Node::childrenLastModified() const
    {
      return properties.childrenMTime;
    }

    void Node::markAsCommitted()
    {
      properties.lastCommitted = TimeStamp();
    }

    void Node::markAsModified()
    {
      properties.lastModified = TimeStamp();
      if (hasParent())
        parent().setChildrenModified(properties.lastModified);
    }

    void Node::setChildrenModified(TimeStamp t)
    {
      if (t > properties.childrenMTime) {
        properties.childrenMTime = t;
        if (hasParent())
          parent().setChildrenModified(properties.childrenMTime);
      }
    }

    // Parent-child structual interface ///////////////////////////////////////

    bool Node::hasChild(const std::string &name) const
    {
      std::string lower=name;
      std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
      std::lock_guard<std::mutex> lock{mutex};
      auto itr = properties.children.find(lower);
      return itr != properties.children.end();
    }

    Node& Node::child(const std::string &name) const
    {
      std::string lower=name;
      std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
      std::lock_guard<std::mutex> lock{mutex};
      auto itr = properties.children.find(lower);
      if (itr == properties.children.end()) {
        throw std::runtime_error("in node " + toString() +
                                 " : could not find sg child node with name '"
                                 + name + "'");
      } else {
        return *itr->second;
      }
    }

    Node& Node::operator[](const std::string &c) const
    {
      return child(c);
    }

    Node& Node::childRecursive(const std::string &name)
    {
      mutex.lock();
      Node* n = this;
      auto f = n->properties.children.find(name);
      if (f != n->properties.children.end()) {
        mutex.unlock();
        return *f->second;
      }

      for (auto &child : properties.children) {
        mutex.unlock();
        try {
          return child.second->childRecursive(name);
        }
        catch (const std::runtime_error &) {}

        mutex.lock();
      }

      mutex.unlock();
      throw std::runtime_error("error finding node in Node::childRecursive");
    }

    std::vector<std::shared_ptr<Node>> Node::children() const
    {
      std::lock_guard<std::mutex> lock{mutex};
      std::vector<std::shared_ptr<Node>> result;
      for (auto &child : properties.children)
        result.push_back(child.second);
      return result;
    }

    void Node::add(std::shared_ptr<Node> node)
    {
      std::lock_guard<std::mutex> lock{mutex};
      const std::string& name = node->name();
      std::string lower=name;
      std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
      properties.children[lower] = node;

      node->setParent(*this);
    }

    Node& Node::operator+=(std::shared_ptr<Node> n)
    {
      add(n);
      return *this;
    }

    Node& Node::createChild(std::string name,
                                std::string type,
                                SGVar var,
                                int flags,
                                std::string documentation)
    {
      auto child = createNode(name, type, var, flags, documentation);
      add(child);
      return *child;
    }

    void Node::setChild(const std::string &name,
                            const std::shared_ptr<Node> &node)
    {
      std::string lower=name;
      std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
      properties.children[lower] = node;
#ifndef _WIN32
# warning "TODO: child node parent needs to be set, which requires multi-parent support"
#endif
    }

    bool Node::hasParent() const
    {
      return properties.parent != nullptr;
    }

    Node& Node::parent() const
    {
      return *properties.parent;
    }

    void Node::setParent(Node &p)
    {
      std::lock_guard<std::mutex> lock{mutex};
      properties.parent = &p;
    }

    void Node::setParent(const std::shared_ptr<Node> &p)
    {
      std::lock_guard<std::mutex> lock{mutex};
      properties.parent = p.get();
    }

    // Traversal interface ////////////////////////////////////////////////////

    void Node::traverse(RenderContext &ctx, const std::string& operation)
    {
      //TODO: make child m time propagate
      if (operation != "verify" && operation != "print" && !isValid())
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

    using CreatorFct = sg::Node*(*)();

    std::map<std::string, CreatorFct> nodeRegistry;

    std::shared_ptr<Node> createNode(std::string name,
                                     std::string type,
                                     SGVar var,
                                     int flags,
                                     std::string documentation)
    {
      std::map<std::string, CreatorFct>::iterator it = nodeRegistry.find(type);
      CreatorFct creator = nullptr;

      if (it == nodeRegistry.end()) {
        std::string creatorName = "ospray_create_sg_node__"+std::string(type);
        creator = (CreatorFct)getSymbol(creatorName);

        if (!creator)
          throw std::runtime_error("unknown ospray scene graph node '"+type+"'");

        nodeRegistry[type] = creator;
      } else {
        creator = it->second;
      }

      std::shared_ptr<sg::Node> newNode(creator());
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
