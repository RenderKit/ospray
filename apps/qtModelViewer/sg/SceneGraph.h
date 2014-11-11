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
#include "sg/geometry/Geometry.h"
#include "sg/geometry/Spheres.h"
#include "sg/geometry/PKD.h"
#include "sg/common/Integrator.h"
#include "sg/camera/Camera.h"

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

    /*! data array */
    template<typename T>
    struct DataVector : public sg::Data {
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::DataArray<T>"; };
      
      std::vector<T> t;
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
      void createFB() { ospFrameBuffer = ospNewFrameBuffer(size,OSP_RGBA_I8,OSP_FB_COLOR|OSP_FB_ACCUM); }
      void destroyFB() { ospFreeFrameBuffer(ospFrameBuffer); }
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
  
