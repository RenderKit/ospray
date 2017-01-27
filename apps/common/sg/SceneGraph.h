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

// std
#include <map>

// sg components
#include "sg/common/Node.h"
#include "sg/common/Integrator.h"
#include "sg/common/Data.h"
#include "sg/common/Transform.h"

#include "sg/camera/PerspectiveCamera.h"

#include "sg/geometry/Geometry.h"
#include "sg/geometry/Spheres.h"

#include "sg/volume/Volume.h"

// ospcommon
#include "ospcommon/FileName.h"

#ifdef _WIN32
#  ifdef ospray_sg_EXPORTS
#    define OSPSG_INTERFACE __declspec(dllexport)
#  else
#    define OSPSG_INTERFACE __declspec(dllimport)
#  endif
#else
#  define OSPSG_INTERFACE
#endif

namespace ospray {
  namespace sg {
    using ospcommon::FileName;
    
    /*! \brief allows for adding semantical info to a model/scene
     graph.  \note will not do anything by itself. */
    struct Info : public sg::Node {
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const override { return "ospray::sg::Info"; };

      std::string permissions;
      std::string acks;
      std::string description;
    };

    struct Group : public sg::Node {
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const override { return "ospray::sg::Group"; };

      /*! 'render' the nodes */
      virtual void render(RenderContext &ctx) override;
      virtual box3f getBounds() override;
      
      std::vector<std::shared_ptr<sg::Node> > children;
    };

    /*! a geometry node - the generic geometry node */
    struct GenericGeometry : public sg::Geometry {
      GenericGeometry(const std::string &type) : Geometry(type) {};

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const override { return "ospray::sg::GenericGeometry"; }
      virtual box3f getBounds() override { return bounds; };

      /*! geometry type, i.e., 'spheres', 'cylinders', 'trianglemesh', ... */
      const std::string type; 
      box3f bounds;
    };

    /*! a instance of another model */
    struct Instance : public sg::Geometry {
      Instance() : Geometry("Instance") {};
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const override { return "ospray::sg::Instance"; }

      //! the model we're instancing
      std::shared_ptr<World>    world;
    };

    /*! a light node - the generic light node */
    struct Light : public sg::Node {
      //! \brief constructor
      Light(const std::string &type) : type(type) {};

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const override { return "ospray::sg::Light"; }

      /*! \brief light type, i.e., 'DirectionalLight', 'PointLight', ... */
      const std::string type; 
    };

    /*! import an OBJ wavefront model, and add its contents to the given world */
    OSPSG_INTERFACE void importOBJ(const std::shared_ptr<World> &world, const FileName &fileName);

    /*! import an PLY model, and add its contents to the given world */
    OSPSG_INTERFACE void importPLY(std::shared_ptr<World> &world, const FileName &fileName);

    /*! import an X3D-format model, and add its contents to the given world */
    OSPSG_INTERFACE void importX3D(const std::shared_ptr<World> &world, const FileName &fileName);

    OSPSG_INTERFACE std::shared_ptr<sg::World> loadOSP(const std::string &fileName);
    OSPSG_INTERFACE std::shared_ptr<sg::World> readXML(const std::string &fileName);
    OSPSG_INTERFACE std::shared_ptr<sg::World> importRIVL(const std::string &fileName);
    OSPSG_INTERFACE std::shared_ptr<sg::World> loadOSG(const std::string &fileName);
    /*! @} */

  } // ::ospray::sg
} // ::ospray
  
