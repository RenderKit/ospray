/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "ospray/common/material.h"
#include "matte_ispc.h"

namespace ospray {
  namespace pathtracer {
    struct Matte : public ospray::Material {
      //! \brief common function to help printf-debugging 
      /*! Every derived class should overrride this! */
      virtual std::string toString() const { return "ospray::pathtracer::Matte"; }
      
      //! \brief commit the material's parameters
      virtual void commit() {
        if (getIE() != NULL) return;

        const vec3f reflectance = getParam3f("reflectance",vec3f(1.f));
        // const float rcpRoughness = rcpf(roughness);

        ispcEquivalent = ispc::PathTracer_Matte_create
          ((const ispc::vec3f&)reflectance);
      }
    };

    OSP_REGISTER_MATERIAL(Matte,PathTracer_Matte);
  }
}
