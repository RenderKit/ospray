/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

// ospray API
#include "ospray/ospray.h"
// ospray 
#include "ospray/common/OspCommon.h"
// STL
#include <map>

namespace ospray {
  namespace xml {
    struct Node;
  } // ospray::xml
  namespace sg {
    /*! base node for every scene graph node */
    struct Node;
    /*! c++ wrapper for an ospray model */
    struct World;
    /*! c++ wrapper for an ospray light source */
    struct Light;
    /*! c++ wrapper for an ospray camera */
    struct Geometry;
    /*! c++ wrapper for an ospray geometry type */
    struct Volume;
    /*! c++ wrapper for an ospray volume type */
    struct Camera;
    /*! c++ wrapper for an ospray renderer */
    struct Renderer;

    /*! forward decl of entity that nodes can write to when writing XML files */
    struct XMLWriter;

    /*! a traverser that implements a visitor design pattern. */
    struct Traverser {
      virtual void traverse(Node     *node)     {};
      virtual void traverse(World    *world)    {};
      virtual void traverse(Geometry *geometry) {};
      virtual void traverse(Volume   *volume)   {};
    };
    

    /*! \brief a parameter to a node (is not in itself a node).
      
      \note This is only the abstract base class, actual instantiations are
      the in the 'ParamT' template. */
    struct Param : public embree::RefCount { 
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

    /*! \brief base node of all scene graph nodes */
    struct Node : public embree::RefCount 
    {
      virtual    std::string toString() const = 0;
      sg::Param *getParam(const std::string &name) const;
      void       addParam(sg::Param *p);
      virtual void setFrom(const xml::Node *const node) {};

      //! just for convenience; add a typed 'setParam' function
      template<typename T>
      inline void setParam(const std::string &name, const T &t) 
      { param[name] = new ParamT<T>(name,t); }

      std::string name;
    protected:
      std::map<std::string,Ref<Param> > param;
    };

    /*! \brief C++ wrapper for a material */
    struct Material : public Node {
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::viewer::sg::Material"; };

      //! a logical name, of no other useful meaning whatsoever
      std::string name; 
      //! indicates the type of material/shader the renderer should use for these parameters
      std::string type;
    };

    /*! \brief C++ wrapper for a 2D Texture */
    struct Texture2D : public Node {
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::viewer::sg::Texture2D"; };

      vec2i       size;
      void       *pixels;
      OSPDataType type;
    };

    struct Info : public sg::Node {
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Info"; };

      std::string permissions;
      std::string acks;
      std::string description;
    };

    /*! data array */
    struct Data : public sg::Node {
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Data"; };

      OSPDataType dataType;
      size_t      numItems;
      void       *data;
    };

    /*! a geometry node - the generic geometry node */
    struct Geometry : public sg::Node {
      Geometry(const std::string &type) : type(type) {};

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Geometry"; }

      /*! geometry type, i.e., 'spheres', 'cylinders', 'trianglemesh', ... */
      const std::string type; 

      virtual box3f getBounds() = 0;
    };

    /*! a geometry node - the generic geometry node */
    struct GenericGeometry : public sg::Geometry {
      GenericGeometry(const std::string &type) : Geometry(type) {};

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::GenericGeometry"; }

      /*! geometry type, i.e., 'spheres', 'cylinders', 'trianglemesh', ... */
      const std::string type; 
      box3f bounds;
      virtual box3f getBounds() { return bounds; };
    };

    /*! a instance of another model */
    struct Instance : public sg::Geometry {
      Instance() : Geometry("Instance") {};
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Instance"; }

      //! the model we're instancing
      Ref<World>    world;
      //! the transformation matrix
      AffineSpace3f xfm;
    };

    /*! a light node - the generic light node */
    struct Light : public sg::Node {
      Light(const std::string &type) : type(type) {};
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Light"; }
      /*! light type, i.e., 'ao', 'obj', 'pathtracer', ... */
      const std::string type; 
    };

    /*! a camera node - the generic camera node */
    struct Camera : public sg::Node {
      Camera(const std::string &type) : type(type) {};
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Camera"; }
      /*! camera type, i.e., 'ao', 'obj', 'pathtracer', ... */
      const std::string type; 
    };

    /*! a renderer node - the generic renderer node */
    struct Renderer : public sg::Node {
      Renderer(const std::string &type) : type(type) {};
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Renderer"; }
      /*! renderer type, i.e., 'ao', 'obj', 'pathtracer', ... */
      const std::string type; 
    };

    /*! a world node */
    struct World : public embree::RefCount {
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::viewer::sg::World"; }

      std::vector<Ref<Light> >    light;
      std::vector<Ref<Camera> >   camera;
      std::vector<Ref<Renderer> > renderer;
      std::vector<Ref<Geometry> > geometry;
    };
      
    World *readXML(const std::string &fileName);
    World *importRIVL(const std::string &fileName);
    World *importSpheres(const std::string &fileName);

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
    }                                                                 

  } // ::ospray::sg
} // ::ospray
  
