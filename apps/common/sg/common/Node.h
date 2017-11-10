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

#include "TimeStamp.h"
#include "Serialization.h"
#include "RenderContext.h"
#include "RuntimeError.h"
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

    using Any = ospcommon::utility::Any;

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
      virtual ~Node();

      // NOTE: Nodes are not copyable nor movable! The operator=() will be used
      //       to assign a Node's _value_, which is different than the
      //       parent/child structure of the Node itself.
      Node(const Node &) = delete;
      Node(Node &&) = delete;
      Node& operator=(const Node &) = delete;
      Node& operator=(Node &&) = delete;

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
      Any         min()           const;
      Any         max()           const;
      NodeFlags   flags()         const;
      std::string documentation() const;

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

      // Parent-child structual interface /////////////////////////////////////

      using NodeLink = std::pair<std::string, std::shared_ptr<Node>>;

      // Children //

      bool hasChild(const std::string &name) const;

      /*! return named child node. */
      Node& child(const std::string &name) const;
      Node& operator[] (const std::string &c) const;

      Node& childRecursive(const std::string &name);
      bool hasChildRecursive(const std::string &name);

      const std::map<std::string, std::shared_ptr<Node>>& children() const;

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

      //! called before committing children during traversal
      virtual void preCommit(RenderContext &ctx);

      //! called after committing children during traversal
      virtual void postCommit(RenderContext &ctx);

    protected:

      struct
      {
        std::string name;
        std::string type;
        std::vector<Any> minmax;
        std::vector<Any> whitelist;
        std::vector<Any> blacklist;
        std::map<std::string, std::shared_ptr<Node>> children;
        Any value;
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
    DECLARE_VALUEAS_SPECIALIZATION(OSPTexture2D)
    DECLARE_VALUEAS_SPECIALIZATION(OSPPixelOp)

#undef DECLARE_VALUEAS_SPECIALIZATION

    // Helper functions ///////////////////////////////////////////////////////

    // Compare //

    template <typename T>
    inline bool compare(const Any& min, const Any& max, const Any& value)
    {
      if (value.get<T>() < min.get<T>() || value.get<T>() > max.get<T>())
        return false;
      return true;
    }

    template <>
    inline bool compare<vec2f>(const Any& min, const Any& max, const Any& value)
    {
      const vec2f &v1 = min.get<vec2f>();
      const vec2f &v2 = max.get<vec2f>();
      const vec2f &v  = value.get<vec2f>();
      return !(v1.x > v.x || v2.x < v.x ||
               v1.y > v.y || v2.y < v.y);
    }

    template <>
    inline bool compare<vec2i>(const Any& min, const Any& max, const Any& value)
    {
      const vec2i &v1 = min.get<vec2i>();
      const vec2i &v2 = max.get<vec2i>();
      const vec2i &v  = value.get<vec2i>();
      return !(v1.x > v.x || v2.x < v.x ||
               v1.y > v.y || v2.y < v.y);
    }

    template <>
    inline bool compare<vec3f>(const Any& min, const Any& max, const Any& value)
    {
      const vec3f &v1 = min.get<vec3f>();
      const vec3f &v2 = max.get<vec3f>();
      const vec3f &v  = value.get<vec3f>();
      return !(v1.x > v.x || v2.x < v.x ||
               v1.y > v.y || v2.y < v.y ||
               v1.z > v.z || v2.z < v.z);
    }

    template <>
    inline bool compare<box3f>(const Any&, const Any&, const Any&)
    {
      return true;// NOTE(jda) - this is wrong, was incorrect before refactoring
    }

    // Commit //

    template <typename T>
    inline void commitNodeValue(Node &)
    {
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
    inline void commitNodeValue<vec2i>(Node &n)
    {
      ospSet2iv(n.parent().valueAs<OSPObject>(),
                n.name().c_str(), &n.valueAs<vec2i>().x);
    }

    template <>
    inline void commitNodeValue<vec3i>(Node &n)
    {
      ospSet3iv(n.parent().valueAs<OSPObject>(),
                n.name().c_str(), &n.valueAs<vec3i>().x);
    }

    template <>
    inline void commitNodeValue<float>(Node &n)
    {
      ospSet1f(n.parent().valueAs<OSPObject>(),
               n.name().c_str(), n.valueAs<float>());
    }

    template <>
    inline void commitNodeValue<vec2f>(Node &n)
    {
      ospSet2fv(n.parent().valueAs<OSPObject>(),
                n.name().c_str(), &n.valueAs<vec2f>().x);
    }

    template <>
    inline void commitNodeValue<vec3f>(Node &n)
    {
      ospSet3fv(n.parent().valueAs<OSPObject>(),
                n.name().c_str(), &n.valueAs<vec3f>().x);
    }

    template <>
    inline void commitNodeValue<vec4f>(Node &n)
    {
      ospSet4fv(n.parent().valueAs<OSPObject>(),
                n.name().c_str(), &n.valueAs<vec4f>().x);
    }

    template <>
    inline void commitNodeValue<std::string>(Node &n)
    {
      ospSetString(n.parent().valueAs<OSPObject>(),
                   n.name().c_str(), n.valueAs<std::string>().c_str());
    }

    // Helper parameter node wrapper //////////////////////////////////////////

    template <typename T>
    struct NodeParam : public Node
    {
      NodeParam() : Node() { setValue(T()); }
      virtual void postCommit(RenderContext &) override
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
