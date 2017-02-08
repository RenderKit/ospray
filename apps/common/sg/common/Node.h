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
#include "sg/common/RuntimeError.h"
// stl
#include <map>
#include <memory>
// xml
#include "../../../common/xml/XML.h"
// ospcommon
#include "ospcommon/vec.h"

namespace ospray {

  namespace sg {

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
      rendering any object. note we INTENTIONALLY do not use
      shared_ptrs here because certain nodes want to set these values
      to 'this', which isn't valid for shared_ptrs */
    struct RenderContext {
      std::shared_ptr<sg::World>      world;      //!< world we're rendering into
      std::shared_ptr<sg::Integrator> integrator; //!< integrator used to create materials etc
      const affine3f  xfm;        //!< affine geometry transform matrix

      //! create a new context
      RenderContext() : xfm(one) {};

      //! create a new context with new transformation matrix
      RenderContext(const RenderContext &other, const affine3f &newXfm)
        : world(other.world), integrator(other.integrator), xfm(newXfm)
      {}
    };

    /*! \brief base node of all scene graph nodes */
    struct Node : public std::enable_shared_from_this<Node>
    {
      Node() : lastModified(1), lastCommitted(0) {};

      virtual    std::string toString() const = 0;

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

      // /*! query given parameter */
      // std::shared_ptr<sg::Param> getParam(const std::string &name) const;

      template<typename Lambda>
      inline void for_each_param(const Lambda &functor)
      { for (auto it=params.begin(); it!=params.end(); ++it) functor(it->second); }
    

      /*! serialize the scene graph - add object to the serialization,
        but don't do anything else to the node(s) */
      virtual void serialize(sg::Serialization::State &state);

      /*! \brief 'render' the object for the first time */
      virtual void render(RenderContext &ctx) {};

      /*! \brief 'commit' updates */
      virtual void commit() {};

      /*! \brief return bounding box in world coordinates.

        This function can be used by the viewer(s) for calibrating
        camera motion, setting default camera position, etc. Nodes
        for which that does not apply can simpy return
        box3f(empty) */
      virtual box3f getBounds() { return box3f(empty); };

      //! return when this node was last modified
      inline TimeStamp getLastModified()  const { return lastModified; };

      //! return when this node was last committed
      inline TimeStamp getLastCommitted() const { return lastCommitted; };

      std::string name;
    protected:
      TimeStamp lastModified;
      TimeStamp lastCommitted;
      std::map<std::string, std::shared_ptr<Param>> params;
    };

    /*! read a given scene graph node from its correspondoing xml node represenation */
    sg::Node *parseNode(xml::Node *node);


    // list of all named nodes - for now use this as a global
    // variable, but eventually we'll need tofind a better way for
    // storing this ... maybe in the world!?
    extern std::map<std::string,std::shared_ptr<sg::Node> > namedNodes;
    std::shared_ptr<sg::Node> findNamedNode(const std::string &name);
    void registerNamedNode(const std::string &name, const std::shared_ptr<sg::Node> &node);


    /*! \brief registers a internal ospray::<ClassName> renderer under
      the externally accessible name "external_name"

      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      renderer. By having this symbol in the shared lib ospray can
      lateron always get a handle to this fct and create an instance
      of this renderer.
    */
#define OSP_REGISTER_SG_NODE(InternalClassName)                         \
    extern "C" std::shared_ptr<ospray::sg::Node>                        \
    ospray_create_sg_node__##InternalClassName()                        \
    {                                                                   \
      return std::make_shared<ospray::sg::InternalClassName>();         \
    }

  } // ::ospray::sg
} // ::ospray


