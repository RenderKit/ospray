/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

/*! \defgroup ospray_render_obj "Wavefront OBJ" Material-based Renderer

  \brief Implements the Wavefront OBJ Material Model
  
  \ingroup ospray_supported_renderers

  This renderer implementes a shading model roughly based on the
  Wavefront OBJ material format. In particular, this renderer
  implements the Material model as implemented in \ref
  ospray::OBJMaterial, and implements a recursive ray tracer on top of
  this mateiral model.

  Note that this renderer is NOT fully compatible with the Wavefront
  OBJ specifications - in particular, we do not support the different
  illumination models as specified in the 'illum' field, and always
  perform full ray tracing with the given material parameters.

*/

// ospray
#include "ospray/render/Renderer.h"
#include "ospray/common/Material.h"

// obj renderer
#include "OBJPointLight.h"

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

