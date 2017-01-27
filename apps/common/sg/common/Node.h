// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
#include "sg/common/RuntimeError.h"
// stl
#include <map>
#include <memory>
// xml
#include "../../../common/xml/XML.h"
// ospcommon
#include "ospcommon/vec.h"
#include "mapbox/variant.hpp"

using namespace mapbox::util;

namespace ospray {

  namespace sg {

    // struct SGVar
    // {
    //   SGVar(int v) { vfloat=v;}
    //   SGVar(std::string v) {vstring=v;}
    //   operator float() { return vfloat; }
    //   operator std::string() { return vstring; }
    //   union 
    //   {
    //     float vfloat;
    //     std::string vstring;
    //   };
    // };
    struct NullType {
    };
    bool operator==(const NullType& lhs, const NullType& rhs);
    typedef variant<NullType, 
      // OSPRenderer, OSPModel, OSPCamera, OSPGeometry, OSPMaterial, OSPFrameBuffer, OSPDataType,
      OSPObject,
      ospcommon::vec3f, ospcommon::vec2f, ospcommon::vec2i, ospcommon::box3f, std::string,
      float, bool, int> SGVar;
    bool isValid(SGVar var);


    /*! forward decl of entity that nodes can write to when writing XML files */
    struct XMLWriter;

    /*! helper macro that adds a new member to a scene graph node, and
      automatically defines all accessor functions for said
      member.  */
#define SG_NODE_DECLARE_MEMBER(type,name,capName)       \
    public:                                             \
    inline type get##capName() const { return name; }   \
  public:                                               \
  inline void set##capName(const type &name) {          \
    this->name = name;                                  \
    this->lastModified = TimeStamp::now();              \
  };                                                    \
  protected:                                            \
  type name;                                            \

    /*! \brief a parameter to a node (is not in itself a node).

      \note This is only the abstract base class, actual instantiations are
      the in the 'ParamT' template. */
    struct Param {
      /*! constructor. the passed name alwasys remains constant */
      Param(const std::string &name) : name(name) {};
      /*! return name of this parameter. the value is in the derived class */
      inline const std::string getName() const { return name; }
      virtual void write(XMLWriter &) { NOTIMPLEMENTED; };
      /*! returns the ospray data type that this node corresponds to */
      virtual OSPDataType getOSPDataType() const = 0;
    protected:
      /*! name of this node */
      const std::string name;
    };

    /*! \brief a concrete parameter to a scene graph node */
    template<typename T>
    struct ParamT : public sg::Param {
      ParamT(const std::string &name, const T &t) : Param(name), value(t) {};
      virtual OSPDataType getOSPDataType() const override;
      virtual void write(XMLWriter &) { NOTIMPLEMENTED; };
      T value;
    };

    /*! class that encapsulate all the context/state required for
      rendering any object */
    struct RenderContext {
      World         *world;      //!< world we're rendering into
      Integrator    *integrator; //!< integrator used to create materials etc
      const affine3f xfm;        //!< affine geometry transform matrix
      OSPRenderer ospRenderer;
      int level;

      //! create a new context
      RenderContext() : world(NULL), integrator(NULL), xfm(one),level(0) {};

      //! create a new context with new transformation matrix
      RenderContext(const RenderContext &other, const affine3f &newXfm)
        : world(other.world), integrator(other.integrator), xfm(newXfm),level(0)
      {}

      TimeStamp MTime;
      TimeStamp getMTime() { return MTime; }
      TimeStamp childMTime;
      TimeStamp getChildMTime() { return childMTime; }
    };

    /*! \brief base node of all scene graph nodes */
    struct Node : public RefCount
    {
      Node() : lastModified(TimeStamp::now()), childrenMTime(TimeStamp::now()), lastCommitted(0), name("NULL"), type("Node"),
      ospHandle(nullptr) {};

      virtual    std::string toString() const {}
      std::shared_ptr<sg::Param> getParam(const std::string &name) const;
      // void       addParam(sg::Param *p);

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
      virtual void setFromXML(const xml::Node *const node, 
                              const unsigned char *binBasePtr);

      //! just for convenience; add a typed 'setParam' function
      template<typename T>
      inline void setParam(const std::string &name, const T &t)
      { params[name] = std::make_shared<ParamT<T>>(name,t); }

      template<typename Lambda>
      inline void for_each_param(const Lambda &functor)
      { for (auto it=params.begin(); it!=params.end(); ++it) functor(it->second); }
    

      /*! serialize the scene graph - add object to the serialization,
        but don't do anything else to the node(s) */
      virtual void serialize(sg::Serialization::State &state);

      /*! \brief 'render' the object for the first time */
      virtual void render(RenderContext &ctx) {};

      /*! \brief 'commit' updates */
      virtual void commit() {} 

      /*! \brief return bounding box in world coordinates.

        This function can be used by the viewer(s) for calibrating
        camera motion, setting default camera position, etc. Nodes
        for which that does not apply can simpy return
        box3f(empty) */
      virtual box3f getBounds() { return box3f(empty); };

      //! return when this node was last modified
      inline TimeStamp getLastModified()  const { return lastModified; };
      inline TimeStamp getChildrenLastModified() { return childrenMTime;}

      //! return when this node was last committed
      inline TimeStamp getLastCommitted() const { return lastCommitted; };
      inline void committed() {lastCommitted=TimeStamp::now();}

      inline void modified() { lastModified = TimeStamp::now(); if (parent.isValid()) parent->setChildrenModified(lastModified);}
      void setChildrenModified(TimeStamp t) { if (t >childrenMTime) {childrenMTime = t; if (parent.isValid()) parent->setChildrenModified(childrenMTime);} }

      class NodeH
      {
      public:
        NodeH(Ref<sg::Node> n=nullptr) : node(n) {}
        Ref<sg::Node> node;
        NodeH& operator[] (std::string c) { return node->getChild(c);}
        NodeH operator+= (NodeH n) { node->add(n); n->setParent(*this); return n;}
        Ref<sg::Node> operator-> ()
        {
          return node;
        }
        bool isValid() const
        {
          return node.ptr != nullptr;
        }
      };

      std::string name;
      std::string type;

      NodeH& getChild(std::string name) { return children[name]; }
      std::vector<NodeH> getChildrenByType(std::string) { std::vector<NodeH> result; return result;}
      std::vector<NodeH> getChildren() { std::vector<NodeH> result; 
          for (auto child : children)
            result.push_back(child.second);
          return result; }
      NodeH& operator[] (std::string c) { return getChild(c);}
      NodeH getParent() { return parent; }
      //TODO: to get the handle, should actually call getValue on valid node
      OSPObject getOSPHandle() { return ospHandle; }
      void setParent(const NodeH& p) { parent = p; }
      std::map<std::string, NodeH > children;
      const SGVar getValue() { return value; }
      template<typename T> const T& getValue() { return value.get<T>(); }
      void setValue(SGVar val) { if (val != value) modified(); value = val; }
      virtual void add(Ref<sg::Node> node) { children[node->name] = NodeH(node); node->setParent(NodeH(this)); }
      virtual void add(NodeH node) { children[node->name] = node; node->setParent(NodeH(this)); }
      virtual void traverse(RenderContext &ctx, const std::string& operation);
      virtual void preTraverse(RenderContext &ctx, const std::string& operation);
      virtual void postTraverse(RenderContext &ctx, const std::string& operation);
      virtual void preCommit(RenderContext &ctx) {}
      virtual void postCommit(RenderContext &ctx);
      void setName(std::string v) { name = v; }
      std::string getName() { return name; }
      std::string getType() { return type; }
      void setType(std::string v) { type = v; }
    protected:
      OSPObject ospHandle;
      SGVar value;
      TimeStamp lastModified;
      TimeStamp childrenMTime;
      TimeStamp lastCommitted;
      std::map<std::string, std::shared_ptr<sg::Param> > params;
      NodeH parent;
    };

    /*! read a given scene graph node from its correspondoing xml node represenation */
    sg::Node *parseNode(xml::Node *node);

    // list of all named nodes - for now use this as a global
    // variable, but eventually we'll need tofind a better way for
    // storing this ... maybe in the world!?
    extern std::map<std::string,Ref<sg::Node> > namedNodes;
    sg::Node *findNamedNode(const std::string &name);
    void registerNamedNode(const std::string &name, Ref<sg::Node> node);

    typedef Node::NodeH NodeH;
    Node::NodeH createNode(std::string name, std::string type="Node", SGVar var=NullType());
    // , std::shared_ptr<sg::Param>=std::make_shared<sg::Param>("none")

    template <typename T>
    struct NodeParamCommit
    {
      static void commit(Node* n);
    };

    template <typename T>
    void NodeParamCommit<T>::commit(Node* n) {
      // ospSet1f(parent->getValue<OSPObject>(), getName().c_str(), getValue<float>());
    }

    template <>
    inline void NodeParamCommit<float>::commit(Node* n) {
      ospSet1f(n->getParent()->getValue<OSPObject>(), n->getName().c_str(), n->getValue<float>());
    }
    template <>
    inline void NodeParamCommit<bool>::commit(Node* n) {
      ospSet1i(n->getParent()->getValue<OSPObject>(), n->getName().c_str(), n->getValue<bool>());
    }
    template <>
    inline void NodeParamCommit<int>::commit(Node* n) {
      ospSet1i(n->getParent()->getValue<OSPObject>(), n->getName().c_str(), n->getValue<int>());
    }
    template <>
    inline void NodeParamCommit<vec3f>::commit(Node* n) {
      ospSet3fv(n->getParent()->getValue<OSPObject>(), n->getName().c_str(), &n->getValue<vec3f>().x);
    }

    template <typename T>
    struct NodeParam : public Node {
      NodeParam() { setValue(T()); }
      virtual void postCommit(RenderContext &ctx) { 
          if (parent.isValid())
          {
            if (parent->getValue().is<OSPObject>() == true) //TODO: generalize to other types of ManagedObject
              NodeParamCommit<T>::commit(this);
          }
      }
    };

    struct Renderable : public Node
    {
      Renderable() { add(createNode("bounds", "box3f")); }
      virtual box3f getBounds() { return bbox; }
      virtual box3f extendBounds(box3f b) { bbox.extend(b); }
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
    extern "C" ospray::sg::Node *ospray_create_sg_node__##InternalClassName() \
    {                                                                   \
      return new ospray::sg::InternalClassName;                         \
    }

#define OSP_REGISTER_SG_NODE_NAME(InternalClassName,Name)               \
    extern "C" ospray::sg::Node *ospray_create_sg_node__##Name()        \
    {                                                                   \
      return new ospray::sg::InternalClassName;                         \
    }

  } // ::ospray::sg
} // ::ospray


