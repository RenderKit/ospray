/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once


// ospray
#include "ospray/render/Renderer.h"
#include "ospray/common/Material.h"

// obj renderer
#include "objpointlight.h"

// system
#include <vector>

namespace ospray {
  struct Camera;
  struct Model;

  namespace obj {
    using embree::TaskScheduler;

    /*! \brief Renderer for the OBJ Wavefront Material/Lighting format 

      See \ref ospray_render_obj
    */
    struct OBJRenderer : public Renderer {
      OBJRenderer();
      virtual std::string toString() const { return "ospray::OBJRenderer"; }

      std::vector<void*> pointLightArray; // the 'IE's of the 'pointLightData'
      std::vector<void*> dirLightArray;   // the 'IE's of the 'dirLightData'
      std::vector<void*> spotLightArray;  // the 'IE's of the 'spotLightData'

      Model    *world;
      Camera   *camera;
      Data     *pointLightData;
      Data     *dirLightData;
      Data     *spotLightData;
      
      virtual void commit();

      /*! \brief create a material of given type */
      virtual Material *createMaterial(const char *type);
      /*! \brief create a light of given type */
      virtual Light    *createLight(const char *type);
    };

  }
}

