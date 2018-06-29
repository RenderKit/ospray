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

#include "Serialization.h"
#include "RenderContext.h"
#include "RuntimeError.h"
#include "../visitor/Visitor.h"
// stl
#include <map>
#include <memory>
#include <mutex>
// ospcommon
#include "ospcommon/utility/Any.h"
#include "ospcommon/utility/TimeStamp.h"
#include "ospcommon/xml/XML.h"
#include "ospcommon/vec.h"
#include "common/OSPCommon.h"

namespace ospray {
  namespace sg {

    using Any       = ospcommon::utility::Any;
    using TimeStamp = ospcommon::utility::TimeStamp;

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
      gui_color = 1 << 6,
      gui_combo = 1 << 7,
      gui_readonly = 1 << 8
    };

    // Base Node class definition /////////////////////////////////////////////

    /*! \brief base node of all scene graph nodes */
    struct OSPSG_INTERFACE Node : public std::enable_shared_from_this<Node>
    {
      Node();
      virtual ~Node();

      // NOTE: Nodes are not copyable nor movable! The operator=() will be used
      //       to assign a Node's _value_, which is different than the
      //       parent/child structure of the Node itself.
      Node(const Node &) = delete;
      Node(Node &&) = delete;
      Node& operator=(const Node &) = delete;
      Node& operator=(Node &&) = delete;

      virtual std::string toString() const;

      /*! serialize the scene graph - add object to the serialization,
        but don't do anything else to the node(s) */
      virtual void serialize(sg::Serialization::State &state);

      template <typename T>
      std::shared_ptr<T> nodeAs(); // static cast (faster, but not safe!)

      template <typename T>
      std::shared_ptr<T> tryNodeAs(); // dynamic cast (slower, but can check)

      // Properties ///////////////////////////////////////////////////////////

      std::string name()          const;
      std::string type()          const;
      Any         min()           const;
      Any         max()           const;
      NodeFlags   flags()         const;
      std::string documentation() const;
      std::vector<Any> whitelist() const;

      void setName(const std::string &v);
      void setType(const std::string &v);
      void setMinMax(const Any& minv, const Any& maxv);
      void setFlags(NodeFlags f);
      void setFlags(int f);
      void setDocumentation(const std::string &s);
      void setWhiteList(const std::vector<Any> &values);
      void setBlackList(const std::vector<Any> &values);

      bool isValid() const;

      virtual bool computeValid();
      virtual bool computeValidMinMax();

      /*! \brief return bounding box in world coordinates.
        Cached on node commit.

        This function can be used by the viewer(s) for calibrating
        camera motion, setting default camera position, etc. Nodes
        for which that does not apply can simpy return
        box3f(empty) */
      virtual box3f bounds() const;

      //! return a unique identifier for each created node
      size_t uniqueID() const;

      // Node stored value (data) interface ///////////////////////////////////

      //! get the value (copy) of the node, without template conversion
      Any value();

      //! returns the value of the node in the desired type
      template <typename T>
      T& valueAs();

      //! returns the value of the node in the desired type
      template <typename T>
      const T& valueAs() const;

      //! return if the value is the given type
      template <typename T>
      bool valueIsType() const;

      //! set the value of the node. Requires strict typecast
      template <typename T>
      void setValue(T val);

      //! set the value via the '=' operator
      template <typename T>
      void operator=(T &&val);

      // Update detection interface ///////////////////////////////////////////

      TimeStamp lastModified() const;
      TimeStamp lastCommitted() const;
      TimeStamp childrenLastModified() const;

      void markAsCommitted();
      virtual void markAsModified();
      virtual void setChildrenModified(TimeStamp t);

      //! Did this Node (or decendants) get modified and not (yet) committed?
      bool subtreeModifiedButNotCommitted() const;

      // Parent-child structual interface /////////////////////////////////////

      using NodeLink = std::pair<std::string, std::shared_ptr<Node>>;

      // Children //

      bool hasChild(const std::string &name) const;

      /*! return named child node. */
      Node& child(const std::string &name) const;
      Node& operator[] (const std::string &c) const;

      Node& childRecursive(const std::string &name);
      std::vector<std::shared_ptr<Node> > childrenRecursive(const std::string &name);
      bool hasChildRecursive(const std::string &name);

      const std::map<std::string, std::shared_ptr<Node>>& children() const;

      bool hasChildren() const;
      size_t numChildren() const;

      void add(std::shared_ptr<Node> node);
      void add(std::shared_ptr<Node> node, const std::string &name);

      void remove(const std::string& name);

      //! just for convenience; add a typed 'setParam' function
      template <typename T>
      Node& createChildWithValue(const std::string &name,
                                 const std::string& type,
                                 const T &t);

      Node& createChild(std::string name,
                        std::string type = "Node",
                        Any value = Any(),
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

      //! Use a custom provided node visitor to visit each node
      template <typename VISITOR_T, typename = is_valid_visitor_t<VISITOR_T>>
      void traverse(VISITOR_T &&visitor, TraversalContext &ctx);

      //! Helper overload to traverse with a default constructed TravesalContext
      template <typename VISITOR_T, typename = is_valid_visitor_t<VISITOR_T>>
      void traverse(VISITOR_T &&visitor);

      //! Invoke a "verify" traversal of the scene graph (and possibly
      //  its children)
      void verify();

      //! Invoke a "commit" traversal of the scene graph (and possibly
      //  its children)
      void commit();

      //! Invoke a "finalize" traversal of the scene graph (and possibly
      //  its children)
      void finalize(RenderContext &ctx);

      //! Invoke a "animate" traversal of the scene graph (and possibly
      //  its children)
      void animate();

    protected:

      //! traverse this node and children with given operation, such as
      //  print,commit,render or custom operations
      virtual void traverse(RenderContext &ctx, const std::string& operation);

      //! Helper overload to use a default constructed RenderContext for root
      //  level traversal
      void traverse(const std::string& operation);

      //! called before traversing children
      virtual void preTraverse(RenderContext &ctx,
                               const std::string& operation,
                               bool& traverseChildren);

      //! called after traversing children
      virtual void postTraverse(RenderContext &ctx,
                                const std::string& operation);

      //! Called before committing children during traversal
      virtual void preCommit(RenderContext &ctx);

      //! Called after committing children during traversal
      virtual void postCommit(RenderContext &ctx);

      struct
      {
        std::string name;
        std::string type;
        std::vector<Any> minmax;
        std::vector<Any> whitelist;
        std::vector<Any> blacklist;
        std::map<std::string, std::shared_ptr<Node>> children;
        Any value;
        TimeStamp whenCreated;
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
      mutable std::mutex value_mutex;

    private:

      friend struct VerifyNodes;
    };

    OSPSG_INTERFACE std::shared_ptr<Node>
    createNode(std::string name,
               std::string type = "Node",
               Any var = Any(),
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

    template <typename T>
    inline std::shared_ptr<T> Node::tryNodeAs()
    {
      static_assert(std::is_base_of<Node, T>::value,
                    "Can only use tryNodeAs<T> to cast to an ospray::sg::Node"
                    " type! 'T' must be a child of ospray::sg::Node!");
      return std::dynamic_pointer_cast<T>(shared_from_this());
    }

    //! just for convenience; add a typed 'setParam' function
    template <typename T>
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

    template <typename T>
    inline void Node::setValue(T _val)
    {
      Any val(_val);
      bool modified = false;
      {
        std::lock_guard<std::mutex> lock{value_mutex};
        if (val != properties.value) {
          properties.value = val;
          modified = true;
        }
      }

      if (modified)
        markAsModified();
    }

    template <>
    inline void Node::setValue(Any val)
    {
      bool modified = false;
      {
        std::lock_guard<std::mutex> lock{value_mutex};
        if (val != properties.value)
        {
          properties.value = val;
          modified = true;
        }
      }

      if (modified)
        markAsModified();
    }

    template <typename T>
    inline T& Node::valueAs()
    {
      std::lock_guard<std::mutex> lock{value_mutex};
      return properties.value.get<T>();
    }

    template <typename T>
    inline const T& Node::valueAs() const
    {
      std::lock_guard<std::mutex> lock{value_mutex};
      return properties.value.get<T>();
    }

    template <typename T>
    inline bool Node::valueIsType() const
    {
      std::lock_guard<std::mutex> lock{value_mutex};
      return properties.value.is<T>();
    }

    template <typename T>
    inline void Node::operator=(T &&v)
    {
      setValue(std::forward<T>(v));
    }

    // NOTE(jda) - Specialize valueAs() and operator=() so we don't have to
    //             convert to/from OSPObject manually, must trust the user to
    //             store/get the right type of OSPObject. This is because
    //             ospcommon::utility::Any<> cannot do implicit conversion...

#define DECLARE_VALUEAS_SPECIALIZATION(a)                                      \
    template <>                                                                \
    inline a& Node::valueAs()                                                  \
    {                                                                          \
      std::lock_guard<std::mutex> lock{value_mutex};                           \
      return (a&)properties.value.get<OSPObject>();                            \
    }                                                                          \
                                                                               \
    template <>                                                                \
    inline const a& Node::valueAs() const                                      \
    {                                                                          \
      std::lock_guard<std::mutex> lock{value_mutex};                           \
      return (const a&)properties.value.get<OSPObject>();                      \
    }                                                                          \
                                                                               \
    template <>                                                                \
    inline void Node::operator=(a &&v)                                         \
    {                                                                          \
      setValue((OSPObject)v);                                                  \
    }                                                                          \
                                                                               \
    template <>                                                                \
    inline void Node::setValue(a val)                                          \
    {                                                                          \
      setValue((OSPObject)val);                                                \
    }

    DECLARE_VALUEAS_SPECIALIZATION(OSPDevice)
    DECLARE_VALUEAS_SPECIALIZATION(OSPFrameBuffer)
    DECLARE_VALUEAS_SPECIALIZATION(OSPRenderer)
    DECLARE_VALUEAS_SPECIALIZATION(OSPCamera)
    DECLARE_VALUEAS_SPECIALIZATION(OSPModel)
    DECLARE_VALUEAS_SPECIALIZATION(OSPData)
    DECLARE_VALUEAS_SPECIALIZATION(OSPGeometry)
    DECLARE_VALUEAS_SPECIALIZATION(OSPMaterial)
    DECLARE_VALUEAS_SPECIALIZATION(OSPLight)
    DECLARE_VALUEAS_SPECIALIZATION(OSPVolume)
    DECLARE_VALUEAS_SPECIALIZATION(OSPTransferFunction)
    DECLARE_VALUEAS_SPECIALIZATION(OSPTexture)
    DECLARE_VALUEAS_SPECIALIZATION(OSPPixelOp)

#undef DECLARE_VALUEAS_SPECIALIZATION

    template <typename VISITOR_T, typename>
    inline void Node::traverse(VISITOR_T &&visitor, TraversalContext &ctx)
    {
      static_assert(is_valid_visitor<VISITOR_T>::value,
                    "VISITOR_T must be a child class of sg::Visitor or"
                    " implement 'bool visit(Node &node, TraversalContext &ctx)'"
                    "!");

      bool traverseChildren = visitor(*this, ctx);

      ctx.level++;

      if (traverseChildren) {
        for (auto &child : properties.children)
          child.second->traverse(visitor, ctx);
      }

      ctx.level--;

      visitor.postChildren(*this, ctx);
    }

    template <typename VISITOR_T, typename>
    inline void Node::traverse(VISITOR_T &&visitor)
    {
      TraversalContext ctx;
      traverse(std::forward<VISITOR_T>(visitor), ctx);
    }

    /*! \brief registers a internal ospray::<ClassName> renderer under
      the externally accessible name "external_name"

      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      renderer. By having this symbol in the shared lib ospray can
      lateron always get a handle to this fct and create an instance
      of this renderer.
    */
#define OSP_REGISTER_SG_NODE_NAME(InternalClassName,Name)               \
    extern "C" OSPRAY_DLLEXPORT ospray::sg::Node*                       \
    ospray_create_sg_node__##Name()                                     \
    {                                                                   \
      return new InternalClassName;                                     \
    }                                                                   \
    /* Extra declaration to avoid "extra ;" pedantic warnings */        \
    ospray::sg::Node* ospray_create_sg_node__##Name()

#define OSP_REGISTER_SG_NODE(InternalClassName)                         \
    OSP_REGISTER_SG_NODE_NAME(InternalClassName, InternalClassName)

  } // ::ospray::sg
} // ::ospray
