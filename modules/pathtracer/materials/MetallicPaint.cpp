/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "ospray/common/Material.h"
#include "metallicpaint_ispc.h"

namespace ospray {
  namespace pathtracer {
    struct MetallicPaint : public ospray::Material {
      //! \brief common function to help printf-debugging 
      /*! Every derived class should overrride this! */
      virtual std::string toString() const { return "ospray::pathtracer::MetallicPaint"; }
      
      //! \brief commit the material's parameters
      virtual void commit() {
        if (getIE() != NULL) return;

        const vec3f& shadeColor
          = getParam3f("shadeColor",vec3f(0.5,0.42,0.35)); //vec3f(0.19,0.45,1.5));
        const vec3f& glitterColor
          = getParam3f("glitterColor",vec3f(0.5,0.44,0.42)); //3.06,2.4,1.88));

        const float glitterSpread
          = getParamf("glitterSpread",0.f);
        const float eta
          = getParamf("eta",1.45f);
        
        ispcEquivalent = ispc::PathTracer_MetallicPaint_create
          ((const ispc::vec3f&)shadeColor,(const ispc::vec3f&)glitterColor,
           glitterSpread, eta);
      }
    };

    OSP_REGISTER_MATERIAL(MetallicPaint,PathTracer_MetallicPaint);
  }
}
