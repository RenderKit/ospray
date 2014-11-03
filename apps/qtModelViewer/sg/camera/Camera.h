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

namespace ospray {
  namespace sg {

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

    
  } // ::ospray::sg
} // ::ospray


