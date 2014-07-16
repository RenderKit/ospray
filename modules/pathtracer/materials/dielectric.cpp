/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "ospray/common/material.h"
#include "dielectric_ispc.h"

namespace ospray {
  namespace pathtracer {
    struct Dielectric : public ospray::Material {
      //! \brief common function to help printf-debugging 
      /*! Every derived class should overrride this! */
      virtual std::string toString() const { return "ospray::pathtracer::Dielectric"; }
      
      //! \brief commit the material's parameters
      virtual void commit() {
        if (getIE() != NULL) return;

        const vec3f& transmission
          = getParam3f("transmission",vec3f(1.f));
        const vec3f& transmissionOutside
          = getParam3f("transmissionOutside",vec3f(1.f));

        const float etaInside
          = getParamf("etaInside",1.4f);
        const float etaOutside
          = getParamf("etaOutside",1.f);
        
        ispcEquivalent = ispc::PathTracer_Dielectric_create
          (etaOutside,(const ispc::vec3f&)transmissionOutside,
           etaInside,(const ispc::vec3f&)transmission);
      }
    };

    OSP_REGISTER_MATERIAL(Dielectric,PathTracer_Dielectric);
  }
}
