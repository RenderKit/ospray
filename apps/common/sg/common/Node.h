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

#pragma once

#include "sg/common/TimeStamp.h"
#include "sg/common/Serialization.h"
#include "sg/common/RenderContext.h"
#include "sg/common/RuntimeError.h"
// stl
#include <map>
#include <memory>
// xml
#include "../../../common/xml/XML.h"
// ospcommon
#include "ospcommon/utility/Any.h"
#include "ospcommon/vec.h"

#include <mutex>

namespace ospray {
  namespace sg {

    using SGVar = ospcommon::utility::Any;

    /*! forward decl of entity that nodes can write to when writing XML files */
    struct XMLWriter;

    enum NodeFlags
    {
      none = 0 << 0,
      required = 1 << 1,  //! this node is required to be valid by its parent
      valid_min_max = 1 << 2,  //! validity determined by minmax range
      valid_whitelist = 1 << 3, //! validity determined by whitelist
      valid_blacklist = 1 << 4,  //! validity determined by blacklist
      gui_slider = 1 << 5,
      gui_color = 1<<6,
      gui_combo = 1<<7
    };

    // Base Node class definition /////////////////////////////////////////////

    /*! \brief base node of all scene graph nodes */
    struct OSPSG_INTERFACE Node : public std::enable_shared_from_this<Node>
    {
      Node();

      virtual std::string toString() const;

      //! \brief Initialize this node's value from given XML node
      /*!
        \detailed This allows a plug-and-play concept where a XML
        file can specify all kind of nodes wihout needing to know
        their actual types: The XML parser only needs to be able to
        create a proper C++ instance of the given node type (the
        OSP_REGISTER_SG_NODE() macro will allow it to do so), and can
        tell the node to parse itself from the given XML content and
        XML children

        \param node The XML node specifying this node's fields

        \param binBasePtr A pointer to an accompanying binary file (if
        existant) that contains additional binary data that the xml
        node fields may point into
      */
      virtual void setFromXML(const xml::Node &node,
                              const unsigned char *binBasePtr);

      /*! serialize the scene graph - add object to the serialization,
        but don't do anything else to the node(s) */
      virtual void serialize(sg::Serialization::State &state);

      template <typename T>
      std::shared_ptr<T> nodeAs();

      // Properties ///////////////////////////////////////////////////////////

      std::string name()          const;
      std::string type()          const;
      SGVar       min()           const;
      SGVar       max()           const;
      NodeFlags   flags()         const;
      std::string documentation() const;

      void setName(const std::string &v);
      void setType(const std::string &v);
      void setMinMax(const SGVar& minv, const SGVar& maxv);
      void setFlags(NodeFlags f);
      void setFlags(int f);
      void setDocumentation(const std::string &s);
      void setWhiteList(const std::vector<SGVar> &values);
      void setBlackList(const std::vector<SGVar> &values);

      bool isValid() const;

      virtual bool computeValid();
      virtual bool computeValidMinMax();

      /*! \brief return bounding box in world coordinates.

        This function can be used by the viewer(s) for calibrating
        camera motion, setting default camera position, etc. Nodes
        for which that does not apply can simpy return
        box3f(empty) */
      virtual box3f bounds() const;

      // Node stored value (data) interface ///////////////////////////////////

      //! get the value of the node, whithout template conversion
      SGVar value();

      //! returns the value of the node in the desired type
      template<typename T>
      T& valueAs();

      //! returns the value of the node in the desired type
      template<typename T>
      const T& valueAs() const;

      //! set the value of the node.  Requires strict typecast
      void setValue(SGVar val);

      // Update detection interface ///////////////////////////////////////////

      TimeStamp lastModified() const;
      TimeStamp lastCommitted() const;
      TimeStamp childrenLastModified() const;

      void markAsCommitted();
      virtual void markAsModified();
      virtual void setChildrenModified(TimeStamp t);

      // Parent-child structual interface /////////////////////////////////////

      // Children //

      bool hasChild(const std::string &name) const;

      /*! return named child node. */
      Node& child(const std::string &name) const;
      Node& operator[] (const std::string &c) const;

      Node& childRecursive(const std::string &name);
      bool hasChildRecursive(const std::string &name);

      std::vector<std::shared_ptr<Node>> children() const;

      size_t numChildren() { return properties.children.size(); }

      std::map<std::string, std::shared_ptr<Node>>& childrenMap();

      //! add node as child of this one
      Node& operator+=(std::shared_ptr<Node> node);
      void add(std::shared_ptr<Node> node);
      void add(std::shared_ptr<Node> node, const std::string &name);

      void remove(const std::string& name);

      //! just for convenience; add a typed 'setParam' function
      template<typename T>
      Node& createChildWithValue(const std::string &name,
                                 const std::string& type,
                                 const T &t);

      Node& createChild(std::string name,
                        std::string type = "Node",
                        SGVar var = SGVar(),
                        int flags = sg::NodeFlags::none,
                        std::string documentation = "");

      void setChild(const std::string &name,
                    const std::shared_ptr<Node> &node);

      // Parent //

      bool hasParent() const;

      Node& parent() const;
      void setParent(Node &p);
      void setParent(const std::shared_ptr<Node>& p);

      // Traversal interface //////////////////////////////////////////////////

      //! traverse this node and children with given operation, such as
      //  print,commit,render or custom operations
      virtual void traverse(RenderContext &ctx, const std::string& operation);

      //! Helper overload to use a default constructed RenderContext for root
      //  level traversal
      void traverse(const std::string& operation);

      //! called before traversing children
      virtual void preTraverse(RenderContext &ctx,
                               const std::string& operation, bool& traverseChildren);

      //! called after traversing children
      virtual void postTraverse(RenderContext &ctx,
                                const std::string& operation);

      //! called before committing children during traversal
      virtual void preCommit(RenderContext &ctx);

      //! called after committing children during traversal
      virtual void postCommit(RenderContext &ctx);

    protected:

      struct
      {
        std::string name;
        std::string type;
        std::vector<SGVar> minmax;
        std::vector<SGVar> whitelist;
        std::vector<SGVar> blacklist;
        std::map<std::string, std::shared_ptr<Node>> children;
        SGVar value;
        TimeStamp lastModified;
        TimeStamp childrenMTime;
        TimeStamp lastCommitted;
        TimeStamp lastVerified;
        Node* parent {nullptr};
        NodeFlags flags;
        bool valid {false};
        std::string documentation;
      } properties;

      // NOTE(jda) - The mutex is 'mutable' because const methods still need
      //             to be able to lock the mutex
      mutable std::mutex mutex;
    };
    
    OSPSG_INTERFACE std::shared_ptr<Node>
    createNode(std::string name,
               std::string type = "Node",
               SGVar var = SGVar(),
               int flags = sg::NodeFlags::none,
               std::string documentation = "");

    // Inlined Node definitions ///////////////////////////////////////////////

    template <typename T>
    inline std::shared_ptr<T> Node::nodeAs()
    {
      static_assert(std::is_base_of<Node, T>::value,
                    "Can only use nodeAs<T> to cast to an ospray::sg::Node"
                    " type! 'T' must be a child of ospray::sg::Node!");
      return std::static_pointer_cast<T>(shared_from_this());
    }

    //! just for convenience; add a typed 'setParam' function
    template<typename T>
    inline Node &Node::createChildWithValue(const std::string &name,
                                            const std::string& type,
                                            const T &t)
    {
      if (hasChild(name)) {
        auto &c = child(name);
        c.setValue(t);
        return c;
      }
      else {
        auto node = createNode(name, type, t);
        add(node);
        return *node;
      }
    }

    template<typename T>
    inline T& Node::valueAs()
    {
      std::lock_guard<std::mutex> lock{mutex};
      return properties.value.get<T>();
    }

    template<typename T>
    inline const T& Node::valueAs() const
    {
      std::lock_guard<std::mutex> lock{mutex};
      return properties.value.get<T>();
    }



    // Helper functions ///////////////////////////////////////////////////////

    // Compare //

    template <typename T>
    inline bool compare(const SGVar& min, const SGVar& max, const SGVar& value)
    {
      if (value.get<T>() < min.get<T>() || value.get<T>() > max.get<T>())
        return false;
      return true;
    }

    template <>
    inline bool compare<vec2f>(const SGVar& min,
                               const SGVar& max,
                               const SGVar& value)
    {
      const vec2f &v1 = min.get<vec2f>();
      const vec2f &v2 = max.get<vec2f>();
      const vec2f &v  = value.get<vec2f>();
      return !(v1.x > v.x || v2.x < v.x ||
               v1.y > v.y || v2.y < v.y);
    }

    template <>
    inline bool compare<vec2i>(const SGVar& min,
                               const SGVar& max,
                               const SGVar& value)
    {
      const vec2i &v1 = min.get<vec2i>();
      const vec2i &v2 = max.get<vec2i>();
      const vec2i &v  = value.get<vec2i>();
      return !(v1.x > v.x || v2.x < v.x ||
               v1.y > v.y || v2.y < v.y);
    }

    template <>
    inline bool compare<vec3f>(const SGVar& min,
                               const SGVar& max,
                               const SGVar& value)
    {
      const vec3f &v1 = min.get<vec3f>();
      const vec3f &v2 = max.get<vec3f>();
      const vec3f &v  = value.get<vec3f>();
      return !(v1.x > v.x || v2.x < v.x ||
               v1.y > v.y || v2.y < v.y ||
               v1.z > v.z || v2.z < v.z);
    }

    template <>
    inline bool compare<box3f>(const SGVar& min,
                               const SGVar& max,
                               const SGVar& value)
    {
      return true;// NOTE(jda) - this is wrong, was incorrect before refactoring
    }

    // Commit //

    template <typename T>
    inline void commitNodeValue(Node &)
    {
    }

    template <>
    inline void commitNodeValue<float>(Node &n)
    {
      ospSet1f(n.parent().valueAs<OSPObject>(),
               n.name().c_str(), n.valueAs<float>());
    }

    template <>
    inline void commitNodeValue<bool>(Node &n)
    {
      ospSet1i(n.parent().valueAs<OSPObject>(),
               n.name().c_str(), n.valueAs<bool>());
    }

    template <>
    inline void commitNodeValue<int>(Node &n)
    {
      ospSet1i(n.parent().valueAs<OSPObject>(),
               n.name().c_str(), n.valueAs<int>());
    }

    template <>
    inline void commitNodeValue<vec3f>(Node &n)
    {
      ospSet3fv(n.parent().valueAs<OSPObject>(),
                n.name().c_str(), &n.valueAs<vec3f>().x);
    }


    template <>
    inline void commitNodeValue<vec2f>(Node &n)
    {
      ospSet3fv(n.parent().valueAs<OSPObject>(),
                n.name().c_str(), &n.valueAs<vec2f>().x);
    }

    // Helper parameter node wrapper //////////////////////////////////////////

    template <typename T>
    struct NodeParam : public Node
    {
      NodeParam() : Node() { setValue(T()); }
      virtual void postCommit(RenderContext &ctx) override
      {
        if (hasParent()) {
          //TODO: generalize to other types of ManagedObject

          //NOTE(jda) - OMG the syntax for the 'if' is strange...
          if (parent().value().template is<OSPObject>())
            commitNodeValue<T>(*this);
        }
      }

      virtual bool computeValidMinMax() override
      {
        if (properties.minmax.size() < 2 ||
            !(flags() & NodeFlags::valid_min_max))
          return true;

        return compare<T>(min(), max(), value());
      }
    };

    // Base Node for all renderables //////////////////////////////////////////

    //! a Node with bounds and a render operation
    struct OSPSG_INTERFACE Renderable : public Node
    {
      Renderable() { createChild("bounds", "box3f", box3f(empty)); }
      virtual ~Renderable() = default;

      virtual std::string toString() const override
      { return "ospray::sg::Renderable"; }

      virtual box3f bounds() const override
      { return child("bounds").valueAs<box3f>(); }

      virtual box3f computeBounds() const
      {
        box3f cbounds = empty;
        for (const auto &child : properties.children)
        {
          auto tbounds = child.second->bounds();
            cbounds.extend(tbounds);
        }
        return cbounds;
      }

      virtual void preTraverse(RenderContext &ctx,
                               const std::string& operation, bool& traverseChildren) override;
      virtual void postTraverse(RenderContext &ctx,
                                const std::string& operation) override;
      virtual void postCommit(RenderContext &ctx) override
      { child("bounds").setValue(computeBounds()); }
      virtual void preRender(RenderContext &ctx)  {}
      virtual void postRender(RenderContext &ctx) {}
    };

    /*! \brief registers a internal ospray::<ClassName> renderer under
      the externally accessible name "external_name"

      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      renderer. By having this symbol in the shared lib ospray can
      lateron always get a handle to this fct and create an instance
      of this renderer.
    */
#define OSP_REGISTER_SG_NODE_NAME(InternalClassName,Name)               \
    extern "C" OSPSG_INTERFACE ospray::sg::Node*                        \
    ospray_create_sg_node__##Name()                                     \
    {                                                                   \
      return new ospray::sg::InternalClassName;                         \
    }                                                                   \
    /* Extra declaration to avoid "extra ;" pedantic warnings */        \
    ospray::sg::Node* ospray_create_sg_node__##Name()

#define OSP_REGISTER_SG_NODE(InternalClassName)                         \
    OSP_REGISTER_SG_NODE_NAME(InternalClassName, InternalClassName)

  } // ::ospray::sg
} // ::ospray
