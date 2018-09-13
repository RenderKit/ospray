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

#include "Node.h"
#include "../visitor/VerifyNodes.h"

#include "ospcommon/utility/StringManip.h"

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
#ifdef _WIN32
#  if _MSC_VER <= 1800
      properties.parent = nullptr;
      properties.valid = false;
#  endif
#endif
      properties.flags = sg::NodeFlags::none;
      markAsModified();
    }

    Node::~Node()
    {
      // Call ospRelease() if the value is an OSPObject handle
      if (valueIsType<OSPObject>())
        ospRelease(valueAs<OSPObject>());
    }

    std::string Node::toString() const
    {
      return "ospray::sg::Node";
    }

    void Node::serialize(sg::Serialization::State &)
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

    Any Node::min() const
    {
      return properties.minmax[0];
    }

    Any Node::max() const
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

    std::vector<Any> Node::whitelist() const
    {
      return properties.whitelist;
    }

    void Node::setName(const std::string &v)
    {
      properties.name = v;
    }

    void Node::setType(const std::string &v)
    {
      properties.type = v;
    }

    void Node::setMinMax(const Any &minv, const Any &maxv)
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

    void Node::setWhiteList(const std::vector<Any> &values)
    {
      properties.whitelist = values;
    }

    void Node::setBlackList(const std::vector<Any> &values)
    {
      properties.blacklist = values;
    }

    bool Node::isValid() const
    {
      return properties.valid;
    }

    bool Node::computeValid()
    {
      bool valid = true;

      if ((flags() & NodeFlags::valid_min_max) &&
          properties.minmax.size() > 1) {
        if (!computeValidMinMax())
          valid = false;
      }

      if (flags() & NodeFlags::valid_blacklist) {
        valid &= std::find(properties.blacklist.begin(),
                         properties.blacklist.end(),
                         value()) == properties.blacklist.end();
      }

      if (flags() & NodeFlags::valid_whitelist) {
        valid &= std::find(properties.whitelist.begin(),
                         properties.whitelist.end(),
                         value()) != properties.whitelist.end();
      }

      return valid;
    }

    bool Node::computeValidMinMax()
    {
      return true;
    }

    box3f Node::bounds() const
    {
      return empty;
    }

    size_t Node::uniqueID() const
    {
      return properties.whenCreated;
    }

    // Node stored value (data) interface /////////////////////////////////////

    Any Node::value()
    {
      std::lock_guard<std::mutex> lock{value_mutex};
      return properties.value;
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

    bool Node::subtreeModifiedButNotCommitted() const
    {
      return (lastModified() > lastCommitted()) ||
             (childrenLastModified() > lastCommitted());
    }

    // Parent-child structual interface ///////////////////////////////////////

    bool Node::hasChild(const std::string &name) const
    {
      auto itr = properties.children.find(name);
      if (itr != properties.children.end())
        return true;

      std::string name_lower = utility::lowerCase(name);

      auto &c = properties.children;
      itr = std::find_if(c.begin(), c.end(), [&](const NodeLink &n){
        return utility::lowerCase(n.first) == name_lower;
      });

      return itr != properties.children.end();
    }

    Node& Node::child(const std::string &name) const
    {
      auto itr = properties.children.find(name);
      if (itr != properties.children.end())
        return *itr->second;

      std::string name_lower = utility::lowerCase(name);

      auto &c = properties.children;
      itr = std::find_if(c.begin(), c.end(), [&](const NodeLink &n){
        return utility::lowerCase(n.first) == name_lower;
      });

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

    bool Node::hasChildRecursive(const std::string &name)
    {
      bool found = hasChild(name);

      for (auto &child : properties.children) {
        try {
          found |= child.second->hasChildRecursive(name);
        }
        catch (const std::runtime_error &) {}
      }

      return found;
    }

    Node& Node::childRecursive(const std::string &name)
    {
      if (hasChild(name))
        return child(name);

      for (auto &child : properties.children) {
        try {
          return child.second->childRecursive(name);
        }
        catch (const std::runtime_error &) {}
      }

      throw std::runtime_error("error finding node in Node::childRecursive");
    }

    std::vector<std::shared_ptr<Node> > Node::childrenRecursive(const std::string &name)
    {
      std::vector<std::shared_ptr<Node> > found;
      if (hasChild(name))
        found.push_back(child(name).shared_from_this());

      for (auto &child : properties.children) {
          std::vector<std::shared_ptr<Node> > found2 = child.second->childrenRecursive(name);
          found.insert(found.end(), found2.begin(),found2.end());
      }
      return found;
    }

    const std::map<std::string, std::shared_ptr<Node>>& Node::children() const
    {
      return properties.children;
    }

    bool Node::hasChildren() const
    {
      return properties.children.size() != 0;
    }

    size_t Node::numChildren() const
    {
      return properties.children.size();
    }

    void Node::add(std::shared_ptr<Node> node)
    {
      add(node, node->name());
    }

    void Node::add(std::shared_ptr<Node> node, const std::string &name)
    {
      setChild(name, node);
      node->setParent(*this);
    }

    void Node::remove(const std::string &name)
    {
      if (hasChild(name)) {
        child(name).properties.parent = nullptr;
        properties.children.erase(name);
      }
    }

    Node& Node::createChild(std::string name,
                            std::string type,
                            Any value,
                            int flags,
                            std::string documentation)
    {
      auto child = createNode(name, type, value, flags, documentation);
      add(child);
      return *child;
    }

    void Node::setChild(const std::string &name,
                        const std::shared_ptr<Node> &node)
    {
      properties.children[name] = node;
      node->setParent(*this);
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
      properties.parent = &p;
    }

    void Node::setParent(const std::shared_ptr<Node> &p)
    {
      properties.parent = p.get();
    }

    // Traversal interface ////////////////////////////////////////////////////

    void Node::verify()
    {
      traverse(VerifyNodes{false});
    }

    void Node::commit()
    {
      traverse("commit");
    }

    void Node::finalize(RenderContext &ctx)
    {
      traverse(ctx, "render");
    }

    void Node::animate()
    {
      traverse("animate");
    }

    void Node::traverse(RenderContext &ctx, const std::string& operation)
    {
      ctx._childMTime = TimeStamp();
      bool traverseChildren = true;
      preTraverse(ctx, operation, traverseChildren);
      ctx.level++;

      if (traverseChildren) {
        for (auto &child : properties.children)
          child.second->traverse(ctx, operation);
      }

      ctx.level--;
      ctx._childMTime = childrenLastModified();
      postTraverse(ctx, operation);
    }

    void Node::traverse(const std::string &operation)
    {
      RenderContext ctx;
      traverse(ctx, operation);
    }

    void Node::preTraverse(RenderContext &ctx,
                           const std::string& operation,
                           bool& traverseChildren)
    {
      if (operation == "commit") {
        if (subtreeModifiedButNotCommitted())
          preCommit(ctx);
        else
          traverseChildren = false;
      }
    }

    void Node::postTraverse(RenderContext &ctx, const std::string& operation)
    {
      if (operation == "commit" && subtreeModifiedButNotCommitted()) {
        postCommit(ctx);
        markAsCommitted();
      }
    }

    void Node::preCommit(RenderContext &)
    {
    }

    void Node::postCommit(RenderContext &)
    {
    }

    // ==================================================================
    // global stuff
    // ==================================================================

    using CreatorFct = sg::Node*(*)();

    static std::map<std::string, CreatorFct> nodeRegistry;

    std::shared_ptr<Node> createNode(std::string name,
                                     std::string type,
                                     Any value,
                                     int flags,
                                     std::string documentation)
    {
      std::map<std::string, CreatorFct>::iterator it = nodeRegistry.find(type);
      CreatorFct creator = nullptr;

      if (it == nodeRegistry.end()) {
        std::string creatorName = "ospray_create_sg_node__" + type;
        creator = (CreatorFct)getSymbol(creatorName);

        if (!creator)
          throw std::runtime_error("unknown OSPRay sg::Node '" + type + "'");

        nodeRegistry[type] = creator;
      } else {
        creator = it->second;
      }

      std::shared_ptr<sg::Node> newNode(creator());
      newNode->setName(name);
      newNode->setType(type);
      newNode->setFlags(flags);
      newNode->setDocumentation(documentation);

      if (value.valid()) newNode->setValue(value);

      return newNode;
    }

    OSP_REGISTER_SG_NODE(Node);

  } // ::ospray::sg
} // ::ospray
