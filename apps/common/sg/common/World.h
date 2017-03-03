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

#include "sg/common/Node.h"
#include "sg/common/Serialization.h"
#include "sg/camera/Camera.h"

namespace ospray {
  namespace sg {

    /*! a world node */
    struct OSPSG_INTERFACE World : public sg::Renderable
    {
      World() = default;

      virtual void init() override 
      {
        add(createNode("bounds", "box3f"));
      }

      /*! \brief returns a std::string with the c++ name of this class */
      virtual std::string toString() const override
      { return "ospray::viewer::sg::World"; }

      //! serialize into given serialization state 
      virtual void serialize(sg::Serialization::State &serialization) override;

      /*! 'render' the object for the first time */
      virtual void render(RenderContext &ctx) override;

      /*! \brief return bounding box in world coordinates.

        This function can be used by the viewer(s) for calibrating
        camera motion, setting default camera position, etc. Nodes
        for which that does not apply can simpy return
        box3f(embree::empty) */
      virtual box3f getBounds() const override;
      virtual void preCommit(RenderContext &ctx) override;
      virtual void postCommit(RenderContext &ctx) override;
      virtual void preRender(RenderContext &ctx) override;
      virtual void postRender(RenderContext &ctx) override;

      OSPModel ospModel {nullptr};
      std::vector<std::shared_ptr<Node>> nodes;
      std::shared_ptr<sg::World> oldWorld;
    };


    struct OSPSG_INTERFACE InstanceGroup : public sg::World
    {
      InstanceGroup() = default;

      virtual void init() override 
      {
        add(createNode("bounds", "box3f"));
        add(createNode("visible", "bool", true));
        add(createNode("position", "vec3f"));
        add(createNode("rotation", "vec3f", vec3f(0),
                       NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider));
        child("rotation")->setMinMax(-vec3f(2*3.15f),vec3f(2*3.15f));
        add(createNode("scale", "vec3f", vec3f(1.f)));
      }

      virtual void preCommit(RenderContext &ctx) override;
      virtual void postCommit(RenderContext &ctx) override;
      virtual void preRender(RenderContext &ctx) override;
      virtual void postRender(RenderContext &ctx) override;

      OSPGeometry ospInstance {nullptr};
      bool instanced {true};
    };
    
  } // ::ospray::sg
} // ::ospray


