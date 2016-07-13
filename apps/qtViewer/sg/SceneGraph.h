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

#ifndef OSP_SG_INCLUDED
#define OSP_SG_INCLUDED 1
// std
#include <map>

// sg components
#include "sg/common/Node.h"
#include "sg/geometry/Geometry.h"
#include "sg/geometry/Spheres.h"
#include "sg/volume/Volume.h"
#include "sg/common/Integrator.h"
#include "sg/camera/PerspectiveCamera.h"
#include "sg/common/Data.h"
#include "sg/common/FrameBuffer.h"

// ospcommon
#include "ospcommon/FileName.h"

namespace ospray {
  namespace sg {
    using ospcommon::FileName;
    
    /*! \brief allows for adding semantical info to a model/scene
     graph.  \note will not do anything by itself. */
    struct Info : public sg::Node {
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Info"; };

      std::string permissions;
      std::string acks;
      std::string description;
    };

    // /*! data array */
    // struct Data : public sg::Node {
    //   /*! \brief returns a std::string with the c++ name of this class */
    //   virtual    std::string toString() const { return "ospray::sg::Data"; };

    //   OSPDataType dataType;
    //   size_t      numItems;
    //   void       *data;
    // };

    // /*! data array */
    // template<typename T>
    // struct DataVector : public sg::Data {
    //   /*! \brief returns a std::string with the c++ name of this class */
    //   virtual    std::string toString() const { return "ospray::sg::DataArray<T>"; };
      
    //   std::vector<T> t;
    // };

    struct Group : public sg::Node {
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Group"; };

      std::vector<Ref<sg::Node> > child;

      /*! 'render' the nodes */
      virtual void render(RenderContext &ctx)
      { 
        for (int i=0;i<child.size();i++) {
          assert(child[i]);
          PRINT(child[i].ptr);
          child[i]->render(ctx); 
        }
      }

      virtual box3f getBounds()
      {
        box3f bounds = empty;
        for (int i=0;i<child.size();i++) {
          assert(child[i].ptr);
          bounds.extend(child[i]->getBounds());
        }
        return bounds;
      }

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
    };

    //! a transformation node
    struct Transform : public sg::Node {
      //! \brief constructor
      Transform(const AffineSpace3f &xfm=one, sg::Node *node = NULL) 
        : Node(), xfm(xfm), node(node) 
      {}

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Transform"; }

      //! \brief the actual (affine) transformation matrix
      AffineSpace3f xfm;

      //! child node we're transforming
      Ref<sg::Node> node;
    };

    /*! a light node - the generic light node */
    struct Light : public sg::Node {
      //! \brief constructor
      Light(const std::string &type) : type(type) {};

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Light"; }

      /*! \brief light type, i.e., 'DirectionalLight', 'PointLight', ... */
      const std::string type; 
    };

    World *readXML(const std::string &fileName);
    World *importRIVL(const std::string &fileName);
    World *importSpheres(const std::string &fileName);

    /*! import an OBJ wavefront model, and add its contents to the given world */
    void importOBJ(const Ref<World> &world, const FileName &fileName);

    /*! import an PLY model, and add its contents to the given world */
    void importPLY(Ref<World> &world, const FileName &fileName);

    /*! import an X3D-format model, and add its contents to the given world */
    void importX3D(const Ref<World> &world, const FileName &fileName);

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
  
#endif
