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

#include "sg/SceneGraphExports.h"
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
#include "ospcommon/vec.h"

#include "sg/common/OSPAny.h"

#include <mutex>

namespace ospray {
  namespace sg {

    using SGVar = OSPAny;

    /*! forward decl of entity that nodes can write to when writing XML files */
    struct XMLWriter;

    /*! helper macro that adds a new member to a scene graph node, and
      automatically defines all accessor functions for said
      member.  */
#define SG_NODE_DECLARE_MEMBER(type,name,capName)       \
  public:                                               \
    inline type get##capName() const { return name; }   \
    inline void set##capName(const type &name) {        \
      this->name = name;                                \
      this->properties.lastModified = TimeStamp();      \
    };                                                  \
  protected:                                            \
  type name                                             \

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

      /*! Node::Handle is a handle to a sg::Node.  It has the benefit
          of supporting some operators without requiring dereferencing
          a pointer. */
      struct OSPSG_INTERFACE Handle
      {
        Handle() = default;
        Handle(const std::shared_ptr<sg::Node> &n) : node(n) {}

        //! return child with name c
        Handle operator[] (const std::string &c) const
        { return node->child(c); }

        Handle operator[] (const char *c) const
        { return node->child(c); }

        //! add child node n to this node
        Handle operator+= (Handle n)
        { get()->add(n); n->setParent(*this); return n;}

        sg::Node* operator->() const { return node.get(); }

        std::shared_ptr<sg::Node> get() const { return node; }

        //! is this handle pointing to a null value?
        bool notNULL() const { return !isNULL(); }
        //! is this handle pointing to a null value?
        bool isNULL() const { return node.get() == nullptr; }

        operator bool() const { return !isNULL(); }

      private:

        // Data members //

        std::shared_ptr<sg::Node> node;
      };

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

      virtual void init(); //intialize children

      /*! serialize the scene graph - add object to the serialization,
        but don't do anything else to the node(s) */
      virtual void serialize(sg::Serialization::State &state);

      /*! \brief 'render' the object for the first time */
      virtual void render(RenderContext &ctx);

      /*! \brief 'commit' updates */
      virtual void commit();

      std::string documentation();

      void setDocumentation(const std::string &s);

      /*! \brief return bounding box in world coordinates.

        This function can be used by the viewer(s) for calibrating
        camera motion, setting default camera position, etc. Nodes
        for which that does not apply can simpy return
        box3f(empty) */
      virtual box3f bounds() const;

      //! return when this node was last modified
      TimeStamp lastModified() const;
      TimeStamp childrenLastModified() const;

      //! return when this node was last committed
      TimeStamp lastCommitted() const;
      void markAsCommitted();

      virtual void markAsModified();

      virtual void setChildrenModified(TimeStamp t);

      bool hasChild(const std::string &name) const;

      /*! return named child node. */
      Handle child(const std::string &name) const;

      //! return named child node
      Handle childRecursive(const std::string &name);

      //! return all children of type
      std::vector<Handle> childrenByType(const std::string &t) const;

      //! return vector of child handles
      std::vector<Handle> children() const;

      //! return child c
      Handle operator[] (const std::string &c) const;

      //! return the parent node
      Handle parent() const;

      //! sets the parent
      void setParent(const Handle& p);

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

      //! add node as child of this one
      virtual void add(std::shared_ptr<Node> node);

      //! add node as child of this one
      virtual void add(Handle node);

      //! just for convenience; add a typed 'setParam' function
      template<typename T>
      void createChildWithValue(const std::string &name, const T &t);

      //! traverse this node and childrend with given operation, such as
      //  print,commit,render or custom operations
      virtual void traverse(RenderContext &ctx, const std::string& operation);

      //! Helper overload to use a default constructed RenderContext for root
      //  level traversal
      void traverse(const std::string& operation);

      //! called before traversing children
      virtual void preTraverse(RenderContext &ctx,
                               const std::string& operation);

      //! called after traversing children
      virtual void postTraverse(RenderContext &ctx,
                                const std::string& operation);

      //! called before committing children during traversal
      virtual void preCommit(RenderContext &ctx);

      //! called after committing children during traversal
      virtual void postCommit(RenderContext &ctx);

      //! name of the node, ie material007.  Should be unique among children
      void setName(const std::string &v);

      //! set type of node, ie Material
      void setType(const std::string &v);

      //! get name of the node, ie material007
      std::string name() const;

      //! type of node, ie Material
      std::string type() const;

      void setFlags(NodeFlags f);
      void setFlags(int f);
      NodeFlags flags() const;

      void setMinMax(const SGVar& minv, const SGVar& maxv);

      SGVar min() const;
      SGVar max() const;

      void setWhiteList(const std::vector<SGVar> &values);
      void setBlackList(const std::vector<SGVar> &values);

      virtual bool isValid();

      virtual bool computeValid();
      virtual bool computeValidMinMax();

      // NOTE(jda) - This needs to be enabled, BAD to have Node users poking
      //             around in data members! Ideally this should be 'private',
      //             but that's a more minor concern...
#if 0
    protected:
#endif

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
        Handle parent;
        NodeFlags flags;
        bool valid {false};
        std::string documentation;
      } properties;

      // NOTE(jda) - The mutex is 'mutable' because const methods still need
      //             to be able to lock the mutex
      mutable std::mutex mutex;
    };

    // Inlined Node definitions ///////////////////////////////////////////////

    //! just for convenience; add a typed 'setParam' function
    template<typename T>
    inline void Node::createChildWithValue(const std::string &name, const T &t)
    {
      auto iter = properties.children.find("name");
      if (iter != std::end(properties.children))
        iter->second->setValue(t);
      else {
        auto node = std::make_shared<Node>();
        node->setValue(t);
        node->setName(name);
        add(node);
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

    OSPSG_INTERFACE Node::Handle createNode(std::string name,
                                            std::string type = "Node",
                                            SGVar var = SGVar(),
                                            int flags = sg::NodeFlags::none,
                                            std::string documentation="");

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
      ospSet1f(n.parent()->valueAs<OSPObject>(),
               n.name().c_str(), n.valueAs<float>());
    }

    template <>
    inline void commitNodeValue<bool>(Node &n)
    {
      ospSet1i(n.parent()->valueAs<OSPObject>(),
               n.name().c_str(), n.valueAs<bool>());
    }

    template <>
    inline void commitNodeValue<int>(Node &n)
    {
      ospSet1i(n.parent()->valueAs<OSPObject>(),
               n.name().c_str(), n.valueAs<int>());
    }

    template <>
    inline void commitNodeValue<vec3f>(Node &n)
    {
      ospSet3fv(n.parent()->valueAs<OSPObject>(),
                n.name().c_str(), &n.valueAs<vec3f>().x);
    }


    template <>
    inline void commitNodeValue<vec2f>(Node &n)
    {
      ospSet3fv(n.parent()->valueAs<OSPObject>(),
                n.name().c_str(), &n.valueAs<vec2f>().x);
    }

    // Helper parameter node wrapper //////////////////////////////////////////

    template <typename T>
    struct NodeParam : public Node
    {
      NodeParam() : Node() { setValue(T()); }
      virtual void postCommit(RenderContext &ctx) override
      {
        if (!parent().isNULL()) {
          //TODO: generalize to other types of ManagedObject

          //NOTE(jda) - OMG the syntax for the 'if' is strange...
          if (parent()->value().template is<OSPObject>())
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
      Renderable() = default;
      virtual ~Renderable() = default;

      virtual void init() override { add(createNode("bounds", "box3f")); }
      virtual box3f bounds() const override { return bbox; }
      virtual box3f extendBounds(box3f b) { bbox.extend(b); return bbox; }
      virtual void preTraverse(RenderContext &ctx,
                               const std::string& operation) override;
      virtual void postTraverse(RenderContext &ctx,
                                const std::string& operation) override;
      virtual void preCommit(RenderContext &ctx) override { bbox = empty; }
      virtual void preRender(RenderContext &ctx)  {}
      virtual void postRender(RenderContext &ctx) {}
    protected:
      box3f bbox;
    };

    /*! \brief registers a internal ospray::<ClassName> renderer under
      the externally accessible name "external_name"

      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      renderer. By having this symbol in the shared lib ospray can
      lateron always get a handle to this fct and create an instance
      of this renderer.
    */
#define OSP_REGISTER_SG_NODE(InternalClassName)                         \
    extern "C" OSPSG_INTERFACE std::shared_ptr<ospray::sg::Node>        \
    ospray_create_sg_node__##InternalClassName()                        \
    {                                                                   \
      return std::make_shared<ospray::sg::InternalClassName>();         \
    }                                                                   \
    /* Extra declaration to avoid "extra ;" pedantic warnings */        \
    std::shared_ptr<ospray::sg::Node>                                   \
    ospray_create_sg_node__##InternalClassName()

#define OSP_REGISTER_SG_NODE_NAME(InternalClassName,Name)               \
    extern "C" OSPSG_INTERFACE std::shared_ptr<ospray::sg::Node>        \
    ospray_create_sg_node__##Name()                                     \
    {                                                                   \
      return std::make_shared<ospray::sg::InternalClassName>();         \
    }                                                                   \
    /* Extra declaration to avoid "extra ;" pedantic warnings */        \
    std::shared_ptr<ospray::sg::Node>                                   \
    ospray_create_sg_node__##Name()

  } // ::ospray::sg
} // ::ospray
