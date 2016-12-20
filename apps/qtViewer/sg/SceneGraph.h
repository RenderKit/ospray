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

// std
#include <map>

// sg components
#include "sg/common/Node.h"
#include "sg/common/Integrator.h"
#include "sg/common/Data.h"

// ospcommon
#include "ospcommon/FileName.h"

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
      
      std::vector<Ref<sg::Node> > child;
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
      Ref<World>    world;
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

    World *readXML(const std::string &fileName);
    World *importRIVL(const std::string &fileName);

    /*! import an OBJ wavefront model, and add its contents to the given world */
    void importOBJ(const Ref<World> &world, const FileName &fileName);

    /*! import an PLY model, and add its contents to the given world */
    void importPLY(Ref<World> &world, const FileName &fileName);

    /*! import an X3D-format model, and add its contents to the given world */
    void importX3D(const Ref<World> &world, const FileName &fileName);

    Ref<sg::World> loadOSG(const std::string &fileName);
    /*! @} */

  } // ::ospray::sg
} // ::ospray
  
