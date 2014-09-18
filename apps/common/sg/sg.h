/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

/*! \file sg.h Simple Scene Graph for OSPRay model viewer (Interface) */

// ospray API
#include "ospray/ospray.h"
// ospray 
#include "ospray/common/ospcommon.h"
// STL
#include <map>

namespace ospray {
  namespace xml {
    struct Node;
  };
  namespace sg {
    /*! base node for every scene graph node */
    struct Node;

    /*! forward decl of entity that nodes can write to when writing XML files */
    struct XMLWriter;

    /*! \brief a parameter to a node (is not in itself a node).
      
      \note This is only the abstract base class, actual instantiations are
      the in the 'ParamT' template. */
    struct Param { 
      /*! constructor. the passed name alwasys remains constant */
      Param(const std::string &name) : name(name) {};
      /*! return name of this parameter. the value is in the derived class */
      inline const std::string &getName() const { return name; }
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
      virtual OSPDataType getOSPDataType() const;
      virtual void write(XMLWriter &) { NOTIMPLEMENTED; }; 
      T value;
    };

    /*! base node of all scene graph nodes */
    struct Node : public embree::RefCount 
    {
      virtual    std::string toString() const = 0;
      sg::Param *getParam(const std::string &name) const;
      void       addParam(sg::Param *p);
      virtual void setFrom(const xml::Node *const node) {};
    protected:
      std::map<std::string,Param *> param;
    };

    struct Info : public sg::Node {
      virtual    std::string toString() const { return "ospray::sg::Info"; };

      std::string permissions;
      std::string acks;
      std::string description;
    };

    /*! data array */
    struct Data : public sg::Node {
      virtual    std::string toString() const { return "ospray::sg::Data"; };

      OSPDataType dataType;
      size_t      numItems;
      void       *data;
    };

    /*! a geometry node - the generic geometry node */
    struct Geometry : public sg::Node {
      Geometry(const std::string &type) : type(type) {};
      virtual    std::string toString() const { return "ospray::sg::Geometry"; }
      /*! geometry type, i.e., 'spheres', 'cylinders', 'trianglemesh', ... */
      const std::string type; 
    };

    /*! a renderer node - the generic renderer node */
    struct Renderer : public sg::Node {
      Renderer(const std::string &type) : type(type) {};
      virtual    std::string toString() const { return "ospray::sg::Renderer"; }
      /*! renderer type, i.e., 'ao', 'obj', 'pathtracer', ... */
      const std::string type; 
    };

    /*! a node directly corresponding to an ospray model */
    struct Model : public sg::Node {
      virtual    std::string toString() const { return "ospray::sg::Model"; }
      std::vector<Ref<Geometry> > geometry;
    };

    struct World : public embree::RefCount {
      std::vector<Ref<sg::Node> > node;
    };
    World *readXML(const std::string &fileName);

  /*! \brief registers a internal ospray::<ClassName> renderer under
      the externally accessible name "external_name" 
      
      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      renderer. By having this symbol in the shared lib ospray can
      lateron always get a handle to this fct and create an instance
      of this renderer.
  */
#define OSP_REGISTER_SG_NODE(InternalClassName)                         \
    extern "C" sg::Node *ospray_create_sg_node__##InternalClassName()   \
    {                                                                   \
      return new ospray::xml::InternalClassName;                        \
    }                                                                   \
  
  };
}
