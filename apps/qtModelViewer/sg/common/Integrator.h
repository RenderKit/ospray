/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

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
      Integrator(const std::string &type) : type(type), ospRenderer(NULL) {};
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Renderer"; }
      /*! renderer type, i.e., 'ao', 'obj', 'pathtracer', ... */
      const std::string type; 
      
      /*! update the current node's fields to ospray - the node must
        already have been 'render'ed once before this can be
          called */
      virtual void commit();
      
      OSPRenderer getOSPHandle() const { return ospRenderer; }

      SG_NODE_DECLARE_MEMBER(Ref<sg::Camera>,camera,Camera);
      SG_NODE_DECLARE_MEMBER(Ref<sg::World>,world,World);
      // void setCamera(Ref<sg::Camera> camera) { if (camera != this->camera) { this->camera = camera; lastModified = __rdtsc(); } }
      // void setWorld(Ref<sg::World> world) { if (world != this->world) { this->world = world; lastModified = __rdtsc(); } }

      
    private:
      OSPRenderer ospRenderer;
      
      // Ref<sg::World> world;
      // Ref<sg::Camera> camera;
    };

    
  } // ::ospray::sg
} // ::ospray


