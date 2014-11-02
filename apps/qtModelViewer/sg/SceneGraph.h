/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include <map>
#include "sg/common/Node.h"

namespace ospray {
  namespace sg {

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
    
    /*! \brief allows for adding semantical info to a model/scene
     graph.  \note will not do anything by itself. */
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
      Camera(const std::string &type) : type(type), ospCamera(NULL) {};
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Camera"; }
      /*! camera type, i.e., 'ao', 'obj', 'pathtracer', ... */
      const std::string type; 

      virtual void create() { 
        if (ospCamera) destroy();
        ospCamera = ospNewCamera(type.c_str());
        commit();
      };
      virtual void commit() {}
      virtual void destroy() {
        if (!ospCamera) return;
        ospRelease(ospCamera);
        ospCamera = 0;
      }

      OSPCamera ospCamera;
    };

    struct PerspectiveCamera : public sg::Camera {     
      PerspectiveCamera() 
        : Camera("perspective"),
          from(0,-1,0), at(0,0,0), up(0,0,1), aspect(1),
          fovy(60)
      {
        create();
      }

      virtual void commit();
      
      SG_NODE_DECLARE_MEMBER(vec3f,from,From);
      SG_NODE_DECLARE_MEMBER(vec3f,at,At);
      SG_NODE_DECLARE_MEMBER(vec3f,up,Up);
      SG_NODE_DECLARE_MEMBER(float,aspect,Aspect);    
      SG_NODE_DECLARE_MEMBER(float,fovy,Fovy);    
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

      void setCamera(Ref<sg::Camera> camera) { if (camera != this->camera) { this->camera = camera; lastModified = __rdtsc(); } }
      void setWorld(Ref<sg::World> world) { if (world != this->world) { this->world = world; lastModified = __rdtsc(); } }

      OSPRenderer ospRenderer;
      virtual void commit();

    private:
      Ref<sg::World> world;
      Ref<sg::Camera> camera;
    };

    /*! \brief a *tabulated* transfer function realized through
        uniformly spaced color and alpha values between which the
        value will be linearly interpolated (similar to a 1D texture
        for each) */
    struct TransferFunction : public sg::Node {

      TransferFunction() : ospTransferFunction(NULL) { setDefaultValues(); }

      //! \brief initialize color and alpha arrays to 'some' useful values
      void setDefaultValues();

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::TransferFunction"; }

      //! \brief creates ospray-side object(s) for this node
      virtual void render(World *world=NULL, 
                          Integrator *integrator=NULL,
                          const affine3f &xfm = embree::one);

      //! \brief Initialize this node's value from given corresponding XML node 
      virtual void setFromXML(const xml::Node *const node);
      virtual void commit();
      
      /*! set a new color map array */
      void setColorMap(const std::vector<vec3f> &colorArray);
      /*! set a new alpha map array */
      void setAlphaMap(const std::vector<float> &alphaArray);

      /*! return the ospray handle for this xfer fct, so we can assign
          it to ospray obejcts that need a reference to the ospray
          version of this xf */
      OSPTransferFunction getOSPHandle() const { return ospTransferFunction; };
    protected:
      OSPTransferFunction ospTransferFunction;
      OSPData ospColorData;
      OSPData ospAlphaData;

      std::vector<vec3f> colorArray;
      std::vector<float> alphaArray;
    };
    
    /*! a world node */
    struct World : public sg::Node {
      World() : ospModel(NULL) {};

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::viewer::sg::World"; }

      //! serialize into given serialization state 
      virtual void serialize(sg::Serialization::State &serialization);

      std::vector<Ref<Node> > node;

      OSPModel ospModel;
      virtual void render(World *world=NULL, 
                          Integrator *integrator=NULL,
                          const affine3f &xfm = embree::one);

      /*! \brief return bounding box in world coordinates.

        This function can be used by the viewer(s) for calibrating
        camera motion, setting default camera position, etc. Nodes
        for which that does not apply can simpy return
        box3f(embree::empty) */
      virtual box3f getBounds() { 
        box3f bounds = embree::empty;
        for (int i=0;i<node.size();i++)
          bounds.extend(node[i]->getBounds());
        return bounds;
      }

    };
      
    World *readXML(const std::string &fileName);
    World *importRIVL(const std::string &fileName);
    World *importSpheres(const std::string &fileName);
    Ref<sg::World> loadOSP(const std::string &fileName);

    /*! @{ some simple testing geometry */
    World *createTestSphere();

    /*! create a sphere geometry representing a cube of numSpheresPerCubeSize^3 spheres */
    World *createTestSphereCube(size_t numSpheresPerCubeSize);
    /*! create a sphere geometry representing a cube of numSpheresPerCubeSize^3 *alpha*-spheres */
    World *createTestAlphaSphereCube(size_t numSpheresPerCubeSize);
    World *createTestCoordFrame();

    World *importCosmicWeb(const char *fileName, size_t maxParticles);

    /*! @} */

  } // ::ospray::sg
} // ::ospray
  
