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

#include "sg/common/Node.h"
#include "sg/common/Serialization.h"
#include "sg/common/World.h"

namespace ospray {
  namespace sg {

    struct Camera;
    struct World;

    /*! a renderer node - the generic renderer node */
    struct Integrator : public sg::Node {
      Integrator(const std::string &type) : type(type), ospRenderer(NULL), spp(1) {};
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Renderer"; }
      /*! renderer type, i.e., 'ao', 'obj', 'pathtracer', ... */
      const std::string type; 
      
      /*! update the current node's fields to ospray - the node must
        already have been 'render'ed once before this can be
          called */
      virtual void commit();
      
      void setSPP(size_t spp);

      OSPRenderer getOSPHandle() const { return ospRenderer; }

      SG_NODE_DECLARE_MEMBER(Ref<sg::Camera>,camera,Camera);
      SG_NODE_DECLARE_MEMBER(Ref<sg::World>,world,World);
      // void setCamera(Ref<sg::Camera> camera) { if (camera != this->camera) { this->camera = camera; lastModified = __rdtsc(); } }
      // void setWorld(Ref<sg::World> world) { if (world != this->world) { this->world = world; lastModified = __rdtsc(); } }

    public:
      OSPRenderer ospRenderer;
      size_t spp;
      // Ref<sg::World> world;
      // Ref<sg::Camera> camera;
    };

    
  } // ::ospray::sg
} // ::ospray


