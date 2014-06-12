#include "ospray/common/material.h"
#include "thindielectric_ispc.h"

namespace ospray {
  namespace pathtracer {
    struct ThinDielectric : public ospray::Material {
      //! \brief common function to help printf-debugging 
      /*! Every derived class should overrride this! */
      virtual std::string toString() const { return "ospray::pathtracer::ThinDielectric"; }
      
      //! \brief commit the material's parameters
      virtual void commit() {
        if (getIE() != NULL) return;

        const vec3f& transmission
          = getParam3f("transmission",vec3f(1.f)); //vec3f(0.19,0.45,1.5));
        const float eta
          = getParamf("eta",1.4f); //vec3f(.4f,0.f,0.f));
        const float thickness
          = getParamf("thickness",1.f);
        
        ispcEquivalent = ispc::PathTracer_ThinDielectric_create
          ((const ispc::vec3f&)transmission,eta,thickness);
      }
    };

    OSP_REGISTER_MATERIAL(ThinDielectric,PathTracer_ThinDielectric);
    OSP_REGISTER_MATERIAL(ThinDielectric,PathTracer_ThinGlass);
  }
}
