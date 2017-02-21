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
#include "sg/common/RuntimeError.h"
// stl
#include <map>
#include <memory>
// xml
#include "../../../common/xml/XML.h"
// ospcommon
#include "ospray/common/OSPCommon.h"
#include "ospcommon/vec.h"
#include "mapbox/variant.hpp"
#include <mutex>
#include <typeinfo>

namespace ospray {
  namespace sg {

    using namespace ospcommon;

    struct OSPSG_INTERFACE NullType {};

    bool operator==(const NullType& lhs, const NullType& rhs);

    using SGVar = mapbox::util::variant<NullType, OSPObject, vec3f, vec2f,
                                        vec2i, box3f, std::string,
                                        float, bool, int>;

    bool OSPSG_INTERFACE isValid(SGVar var);

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
      this->lastModified = TimeStamp();                 \
    };                                                  \
  protected:                                            \
  type name                                             \

    /*! \brief a parameter to a node (is not in itself a node).

      \note This is only the abstract base class, actual instantiations are
      the in the 'ParamT' template. */
    struct OSPSG_INTERFACE Param
    {
      /*! constructor. the passed name alwasys remains constant */
      Param(const std::string &name) : name(name) {}
      /*! return name of this parameter. the value is in the derived class */
      inline const std::string getName() const { return name; }
      virtual void write(XMLWriter &) { NOTIMPLEMENTED; }
      /*! returns the ospray data type that this node corresponds to */
      virtual OSPDataType getOSPDataType() const = 0;

    protected:
      /*! name of this node */
      const std::string name;
    };

    /*! \brief a concrete parameter to a scene graph node */
    template<typename T>
    struct ParamT : public sg::Param
    {
      ParamT(const std::string &name, const T &t) : Param(name), value(t) {}
      virtual OSPDataType getOSPDataType() const override {return OSP_UNKNOWN;}
      virtual void write(XMLWriter &) { NOTIMPLEMENTED; }
      T value;
    };

    /*! class that encapsulate all the context/state required for
      rendering any object. note we INTENTIONALLY do not use
      shared_ptrs here because certain nodes want to set these values
      to 'this', which isn't valid for shared_ptrs */
    struct OSPSG_INTERFACE RenderContext
    {
      std::shared_ptr<sg::World>      world;      //!< world we're rendering into
      std::shared_ptr<sg::Integrator> integrator; //!< integrator used to create materials etc
      const affine3f  xfm {one};        //!< affine geometry transform matrix
      OSPRenderer ospRenderer {nullptr};
      int level {0};

      RenderContext() = default;

      //! create a new context with new transformation matrix
      RenderContext(const RenderContext &other, const affine3f &newXfm)
        : world(other.world), integrator(other.integrator), xfm(newXfm),level(0),
        ospRenderer(nullptr)
      {}

      TimeStamp MTime;
      TimeStamp getMTime() { return MTime; }
      TimeStamp childMTime;
      TimeStamp getChildMTime() { return childMTime; }
    };

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

    /*! \brief base node of all scene graph nodes */
    struct OSPSG_INTERFACE Node : public std::enable_shared_from_this<Node>
    {
      Node() : name("NULL"), type("Node"), valid(false), ospHandle(nullptr) {}

      /*!
          NodeH is a handle to a sg::Node.  It has the benefit of supporting
          some operators without requiring dereferencing a pointer.
      */
      class OSPSG_INTERFACE NodeH
      {
      public:
        NodeH() = default;
        NodeH(std::shared_ptr<sg::Node> n) : node(n) { nid = 1; }

        size_t nid {0};
        std::shared_ptr<sg::Node> node;

        //! return child with name c
        NodeH& operator[] (std::string c) { return get()->getChild(c);}

        //! add child node n to this node
        NodeH operator+= (NodeH n) { get()->add(n); n->setParent(*this); return n;}

        std::shared_ptr<sg::Node> operator->() { return get(); }

        std::shared_ptr<sg::Node> get() { return isNULL()? nullptr : node; }

        //! is this handle pointing to a null value?
        bool isNULL() const { return nid == 0 || !node; }
      };

      virtual std::string toString() const { return "Node"; }
      std::shared_ptr<sg::Param> getParam(const std::string &name) const;

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

      //! just for convenience; add a typed 'setParam' function
      template<typename T>
      inline void setParam(const std::string &name, const T &t)
      { params[name] = std::make_shared<ParamT<T>>(name,t); }

      template<typename FCN_T>
      inline void for_each_param(FCN_T &&fcn)
      { for (auto &p : params) fcn(p.second); }

      virtual void init() {} //intialize children

      /*! serialize the scene graph - add object to the serialization,
        but don't do anything else to the node(s) */
      virtual void serialize(sg::Serialization::State &state);

      /*! \brief 'render' the object for the first time */
      virtual void render(RenderContext &ctx) {}

      /*! \brief 'commit' updates */
      virtual void commit() {}

      std::string getDocumentation() { return documentation; }

      void setDocumentation(std::string s) { documentation = s; }

      /*! \brief return bounding box in world coordinates.

        This function can be used by the viewer(s) for calibrating
        camera motion, setting default camera position, etc. Nodes
        for which that does not apply can simpy return
        box3f(empty) */
      virtual box3f getBounds() { return box3f(empty); }

      //! return when this node was last modified
      inline TimeStamp getLastModified()  const { return lastModified; }
      inline TimeStamp getChildrenLastModified() { return childrenMTime;}

      //! return when this node was last committed
      inline TimeStamp getLastCommitted() const { return lastCommitted; }
      inline void committed() { lastCommitted = TimeStamp(); }

      virtual void modified()
      {
        lastModified = TimeStamp();
        if (!parent.isNULL()) 
          parent->setChildrenModified(lastModified);
      }

      virtual void setChildrenModified(TimeStamp t)
      {
        if (t >childrenMTime)
        {
          childrenMTime = t; 
          if (!parent.isNULL()) 
            parent->setChildrenModified(childrenMTime);
        } 
      }

      //! return named child node
      NodeH& getChild(std::string name) {
        std::lock_guard<std::mutex> lock{mutex};
        if (children.find(name) == children.end())
          std::cout << "couldn't find child! " << name << "\n";
        return children[name];
      }

      //! return named child node
      NodeH getChildRecursive(std::string name) {
        mutex.lock();
        Node* n = this;
        auto f = n->children.find(name);
        if (f != n->children.end())
        {
          mutex.unlock();
          return f->second;
        }
        for (auto child : children)
        {
          mutex.unlock();
          NodeH r = child.second->getChildRecursive(name);
          if (!r.isNULL())
            return r;
          mutex.lock();
        }
        mutex.unlock();
        return NodeH();
      }

      //! return all children of type
      std::vector<NodeH> getChildrenByType(std::string t)
      {
        std::lock_guard<std::mutex> lock{mutex};
        std::vector<NodeH> result;
        NOT_IMPLEMENTED;
        return result;
      }

      //! return vector of child handles
      std::vector<NodeH> getChildren()
      {
        std::lock_guard<std::mutex> lock{mutex};
        std::vector<NodeH> result;
        for (auto child : children)
          result.push_back(child.second);
        return result;
      }

      //! return child c
      NodeH& operator[] (std::string c) { return getChild(c);}

      //! return the parent node
      NodeH getParent() { return parent; }
      //! returns handle to ospObject, however should actually call getValue on valid node instead
      OSPObject getOSPHandle() { return ospHandle; }

      //! sets the parent
      void setParent(const NodeH& p)
      { std::lock_guard<std::mutex> lock{mutex}; parent = p; }

      //! get the value of the node, whithout template conversion
      const SGVar getValue()
      { std::lock_guard<std::mutex> lock{mutex}; return value; }

      //! returns the value of the node in the desired type
      template<typename T> const T& getValue()
      { std::lock_guard<std::mutex> lock{mutex}; return value.get<T>(); }

      //! set the value of the node.  Requires strict typecast
      void setValue(SGVar val)
      {
        if (val != value)  {
          {
            std::lock_guard<std::mutex> lock{mutex};
            if (val != value)
              value = val;
          }
          modified();
        }
      }

      //! add node as child of this one
      virtual void add(std::shared_ptr<Node> node)
      {
        std::lock_guard<std::mutex> lock{mutex};
        children[node->name] = NodeH(node);

        //ARG!  Cannot call shared_from_this in constructors.  PIA!!!
        node->setParent(shared_from_this());
      }

      //! add node as child of this one
      virtual void add(NodeH node) {
        std::lock_guard<std::mutex> lock{mutex};
        children[node->name] = node;
        node->setParent(shared_from_this());
      }

      //! traverse this node and childrend with given operation, such as
      //  print,commit,render or custom operations
      virtual void traverse(RenderContext &ctx, const std::string& operation);

      //! called before traversing children
      virtual void preTraverse(RenderContext &ctx, const std::string& operation);

      //! called after traversing children
      virtual void postTraverse(RenderContext &ctx, const std::string& operation);

      //! called before committing children during traversal
      virtual void preCommit(RenderContext &ctx) {}

      //! called after committing children during traversal
      virtual void postCommit(RenderContext &ctx) {}

      //! name of the node, ie material007.  Should be unique among children
      void setName(std::string v) { name = v; }

      //! set type of node, ie Material
      void setType(std::string v) { type = v; }

      //! get name of the node, ie material007
      std::string getName() { return name; }

      //! type of node, ie Material
      std::string getType() { return type; }

      void setFlags(NodeFlags f) { flags = f; }
      void setFlags(int f) { flags = static_cast<NodeFlags>(f); }
      NodeFlags getFlags() { return flags; }

      void setMinMax(const SGVar& minv, const SGVar& maxv)
      {
        minmax.resize(2);
        minmax[0]=minv;
        minmax[1]=maxv;
      }

      SGVar getMin() { return minmax[0]; }
      SGVar getMax() { return minmax[1]; }

      void setWhiteList(std::vector<SGVar> values)
      {
        whitelist = values;
      }

      void setBlackList(std::vector<SGVar> values)
      {
        blacklist = values;
      }

      virtual bool isValid() { return valid; }

      virtual bool computeValid()
      {
        if ((flags & NodeFlags::valid_min_max) && minmax.size() > 1) {
          if (!computeValidMinMax())
            return false;
        }
        if (flags & NodeFlags::valid_blacklist) {
          return std::find(blacklist.begin(),
                           blacklist.end(),
                           value) == blacklist.end();
        }
        if (flags & NodeFlags::valid_whitelist) {
          return std::find(whitelist.begin(),
                           whitelist.end(),
                           value) != whitelist.end();
        }

        return true;
      }

      virtual bool computeValidMinMax() { return true; }

      static std::vector<std::shared_ptr<sg::Node>> nodes;

    protected:
      std::string name;
      std::string type;
      std::vector<SGVar> minmax;
      std::vector<SGVar> whitelist;
      std::vector<SGVar> blacklist;
      std::map<std::string, NodeH> children;
      OSPObject ospHandle;
      SGVar value;
      TimeStamp lastModified;
      TimeStamp childrenMTime;
      TimeStamp lastCommitted;
      std::map<std::string, std::shared_ptr<sg::Param>> params;
      NodeH parent;
      std::mutex mutex;
      NodeFlags flags;
      bool valid;
      std::string documentation;
    };

    /*! read a given scene graph node from its correspondoing xml node represenation */
    OSPSG_INTERFACE sg::Node* parseNode(xml::Node *node);

    // list of all named nodes - for now use this as a global
    // variable, but eventually we'll need tofind a better way for
    // storing this ... maybe in the world!?
    extern std::map<std::string,std::shared_ptr<sg::Node>> namedNodes;
    std::shared_ptr<sg::Node> OSPSG_INTERFACE findNamedNode(const std::string &name);
    OSPSG_INTERFACE void registerNamedNode(const std::string &name,
                                           const std::shared_ptr<sg::Node> &node);

    using NodeH = Node::NodeH;
    OSPSG_INTERFACE Node::NodeH createNode(std::string name,
                                           std::string type = "Node",
                                           SGVar var = NullType(),
                                           int flags = sg::NodeFlags::none,
                                           std::string documentation="");

    template <typename T>
    struct NodeParamCommit
    {
      static void commit(std::shared_ptr<Node> n);
      static bool compare(const SGVar& min,
                          const SGVar& max,
                          const SGVar& value);
    };

    template <typename T>
    void NodeParamCommit<T>::commit(std::shared_ptr<Node> n)
    {
    }

    template <typename T>
    bool NodeParamCommit<T>::compare(const SGVar& min,
                                     const SGVar& max,
                                     const SGVar& value)
    {
      return true;
    }

    template<typename T>
    bool NodeParamCommitComparison(const SGVar& min,
                                   const SGVar& max,
                                   const SGVar& value)
    {
      if (value.get<T>() < min.get<T>() || value.get<T>() > max.get<T>())
        return false;
      return true;
    }

    template <>
    inline bool NodeParamCommit<float>::compare(const SGVar& min,
                                                const SGVar& max,
                                                const SGVar& value)
    {
      return NodeParamCommitComparison<float>(min,max,value);
    }

    template <>
    inline void NodeParamCommit<float>::commit(std::shared_ptr<Node> n)
    {
      ospSet1f(n->getParent()->getValue<OSPObject>(),
               n->getName().c_str(), n->getValue<float>());
    }

    template <>
    inline void NodeParamCommit<bool>::commit(std::shared_ptr<Node> n)
    {
      ospSet1i(n->getParent()->getValue<OSPObject>(),
               n->getName().c_str(), n->getValue<bool>());
    }

    template <>
    inline bool NodeParamCommit<int>::compare(const SGVar& min,
                                              const SGVar& max,
                                              const SGVar& value)
    {
      return NodeParamCommitComparison<int>(min,max,value);
    }

    template <>
    inline void NodeParamCommit<int>::commit(std::shared_ptr<Node> n)
    {
      ospSet1i(n->getParent()->getValue<OSPObject>(),
               n->getName().c_str(), n->getValue<int>());
    }

    template <>
    inline bool NodeParamCommit<vec3f>::compare(const SGVar& min,
                                                const SGVar& max,
                                                const SGVar& value)
    {
      const vec3f v1 = min.get<vec3f>();
      const vec3f v2 = max.get<vec3f>();
      const vec3f v = value.get<vec3f>();
      return !(v1.x > v.x || v2.x < v.x ||
               v1.y > v.y || v2.y < v.y ||
               v1.z > v.z || v2.z < v.z);
    }

    template <>
    inline void NodeParamCommit<vec3f>::commit(std::shared_ptr<Node> n)
    {
      ospSet3fv(n->getParent()->getValue<OSPObject>(),
                n->getName().c_str(), &n->getValue<vec3f>().x);
    }

    template <typename T>
    struct NodeParam : public Node
    {
      NodeParam() : Node() { setValue(T()); }
      virtual void postCommit(RenderContext &ctx) override
      {
        if (!parent.isNULL())
        {
          //TODO: generalize to other types of ManagedObject
          if (parent->getValue().is<OSPObject>() == true)
            NodeParamCommit<T>::commit(shared_from_this());
        }
      }

      virtual bool computeValidMinMax() override
      {
        if (minmax.size() < 2 || !(flags & NodeFlags::valid_min_max))
          return true;
        return NodeParamCommit<T>::compare(minmax[0], minmax[1], value);
      }
    };

    //! a Node with bounds and a render operation
    struct OSPSG_INTERFACE Renderable : public Node
    {
      Renderable() = default;
      virtual ~Renderable() = default;

      virtual void init() override { add(createNode("bounds", "box3f")); }
      virtual box3f getBounds() { return bbox; }
      virtual box3f extendBounds(box3f b) { bbox.extend(b); return bbox; }
      virtual void preTraverse(RenderContext &ctx, const std::string& operation);
      virtual void postTraverse(RenderContext &ctx, const std::string& operation);
      virtual void preCommit(RenderContext &ctx) { bbox = empty; }
      virtual void preRender(RenderContext &ctx) {}
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
    extern "C" OSPRAY_DLLEXPORT std::shared_ptr<ospray::sg::Node>       \
    ospray_create_sg_node__##InternalClassName()                        \
    {                                                                   \
      return std::make_shared<ospray::sg::InternalClassName>();         \
    }                                                                   \
    /* Extra declaration to avoid "extra ;" pedantic warnings */        \
    std::shared_ptr<ospray::sg::Node>                                   \
    ospray_create_sg_node__##InternalClassName()

#define OSP_REGISTER_SG_NODE_NAME(InternalClassName,Name)               \
    extern "C" OSPRAY_DLLEXPORT std::shared_ptr<ospray::sg::Node>       \
    ospray_create_sg_node__##Name()                                     \
    {                                                                   \
      return std::make_shared<ospray::sg::InternalClassName>();         \
    }                                                                   \
    /* Extra declaration to avoid "extra ;" pedantic warnings */        \
    std::shared_ptr<ospray::sg::Node>                                   \
    ospray_create_sg_node__##Name()

  } // ::ospray::sg
} // ::ospray
