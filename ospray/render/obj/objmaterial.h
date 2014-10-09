/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "ospray/common/Material.h"
#include "ospray/texture/texture2d.h"

namespace ospray {
  namespace obj {
    typedef vec3f Color;
    
    /*! implements the Material used by the \ref ospray_render_obj */
    struct OBJMaterial : public Material {
      Texture2D *map_d;   float d;     /*! opacity: 0 (transparent), 1 (opaque)                */
      Texture2D *map_Kd;  Color Kd;    /*! diffuse  reflectance: 0 (none), 1 (full)            */
      Texture2D *map_Ks;  Color Ks;    /*! specular reflectance: 0 (none), 1 (full)            */
      Texture2D *map_Ns;  float Ns;    /*! specular exponent: 0 (diffuse), infinity (specular) */
      Texture2D *map_Bump;             /*! bump map */

      //! \brief common function to help printf-debugging 
      /*! Every derived class should overrride this! */
      virtual std::string toString() const { return "ospray::objrenderer::OBJMaterial"; }

      //! \brief commit the material's parameters
      virtual void commit();

      //! desctructor, to clean up
      virtual ~OBJMaterial();
    };
  }
}
