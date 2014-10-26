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
    /*! c++ wrapper for an ospray renderer (the thing that produces
        pixel colors is actualy close to what PBRT calls in
        'integrator' */
    struct Integrator;

    /*! forward decl of entity that nodes can write to when writing XML files */
    struct XMLWriter;

#if 0
    /*! a traverser that implements a visitor design pattern. */
    struct RendererBase : public embree::RefCount {
      virtual void traverse(Node     *node);
      virtual void traverse(World    *world);
      virtual void traverse(Geometry *geometry);
      virtual void traverse(Volume   *volume);

      /*! list of all integrators in the root world */
      std::vector<Ref<sg::Integrator> > integrators;
      // /*! list of all frame buffers defined in the root world */
      // std::vector<Ref<sg::Frame> > frameBuffers;
      /*! the world we are traversing */
      Ref<sg::World> world;
    };
#endif

    /*! class one can use to serialize all nodes in the scene graph */
    struct Serialization {
      typedef enum { 
        /*! when serializing the scene graph, traverse through all
         instances and record each and every occurrence of any object
         (ie, an instanced object will appear multiple times in the
         output, each with one instantiation info */
        DO_FOLLOW_INSTANCES, 
        /*! when serializing the scene graph, record each instanced
            object only ONCE, and list all its instantiations in its
            instantiation vector */
        DONT_FOLLOW_INSTANCES
      } Mode;
      struct Instantiation : public embree::RefCount {
        Ref<Instantiation> parentWorld;
        affine3f           xfm;

        Instantiation() : parentWorld(NULL), xfm(embree::one) {}
      };
      /*! describes one object that we encountered */
      struct Object : public embree::RefCount {
        /*! the node itself */
        Ref<sg::Node>      node;  
        /*! the instantiation info when we traversed this node. May be
          NULL if object isn't instanced, and may contain more than
          one node in case true instancing is being done. */
        std::vector<Ref<Instantiation> > instantiation;
      };

      /*! the node that maintains all the traversal state when
          traversing the scene graph */
      struct State {
        Ref<Instantiation> instantiation;
        Serialization *serialization;
      };

      void serialize(Ref<sg::World> world, Serialization::Mode mode);
      
      /*! clear all old objects */
      void clear() {  object.clear(); }

      size_t size() const { return object.size(); }

      /*! the vector containing all the objects encountered when
          serializing the entire scene graph */
      std::vector<Ref<Object> > object;
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

      virtual void serialize(sg::Serialization::State &serialization);

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

    struct PerspectiveCamera : public sg::Camera {     
      PerspectiveCamera() 
        : Camera("perspective"),
          from(0,-1,0), at(0,0,0), up(0,0,1),
          fovy(60)
      {}

      vec3f from;
      vec3f at;
      vec3f up;
      float fovy;
    };

    struct FrameBuffer : public sg::Node {
      vec2i size;
      OSPFrameBuffer ospFrameBuffer;

      FrameBuffer(const vec2i &size) 
        : size(size), 
          ospFrameBuffer(NULL) 
      { createFB(); };

      unsigned char *map() { return (unsigned char *)ospMapFrameBuffer(ospFrameBuffer, OSP_FB_COLOR); }
      void unmap(unsigned char *mem) { ospUnmapFrameBuffer(mem,ospFrameBuffer); }

      void clearAccum() 
      {
        ospFrameBufferClear(ospFrameBuffer,OSP_FB_ACCUM);
      }
      
      vec2i getSize() const { return size; }

      virtual ~FrameBuffer() { destroyFB(); }

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::FrameBuffer"; }
      
    private:
      void createFB() { ospFrameBuffer = ospNewFrameBuffer(size,OSP_RGBA_I8); }
      void destroyFB() { ospFreeFrameBuffer(ospFrameBuffer); }
    };

    /*! a renderer node - the generic renderer node */
    struct Integrator : public sg::Node {
      Integrator(const std::string &type) : type(type), ospRenderer(NULL) {};
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Renderer"; }
      /*! renderer type, i.e., 'ao', 'obj', 'pathtracer', ... */
      const std::string type; 

      OSPRenderer ospRenderer;
    };

    /*! simple spheres, with all of the key info - position, radius,
        and a int32 type specifier baked into each sphere  */
    struct Spheres : public sg::Geometry {
      struct Sphere { 
        vec3f position;
        float radius;
        uint32 typeID;
        
        Sphere(vec3f position, 
               float radius,
               uint typeID=0) 
          : position(position), 
            radius(radius), 
            typeID(typeID) 
        {}
        inline box3f getBounds() const
        { return box3f(position-vec3f(radius),position+vec3f(radius)); };
      };
      /*! one material per typeID */
      std::vector<Ref<sg::Material> > material;
      std::vector<Sphere>             sphere;

      Spheres() : Geometry("spheres") {};

      //! serialize into given serialization state 
      virtual void serialize(sg::Serialization::State &serialization)
      { PING; }

      virtual box3f getBounds() {
        box3f bounds = embree::empty;
        for (size_t i=0;i<sphere.size();i++)
          bounds.extend(sphere[i].getBounds());
        return bounds;
      }
      
    };
    
    /*! a world node */
    struct World : public sg::Node {
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::viewer::sg::World"; }

      //! serialize into given serialization state 
      virtual void serialize(sg::Serialization::State &serialization);

      std::vector<Ref<Node> > node;
    };
      
    World *readXML(const std::string &fileName);
    World *importRIVL(const std::string &fileName);
    World *importSpheres(const std::string &fileName);
    World *createTestSphere();

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
  
