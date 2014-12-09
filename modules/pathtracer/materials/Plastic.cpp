#include "ospray/common/Material.h"
#include "Plastic_ispc.h"

namespace ospray {
  namespace pathtracer {
    struct Plastic : public ospray::Material {
      //! \brief common function to help printf-debugging 
      /*! Every derived class should overrride this! */
      virtual std::string toString() const { return "ospray::pathtracer::Plastic"; }
      
      //! \brief commit the material's parameters
      virtual void commit() {
        if (getIE() != NULL) return;

        const vec3f pigmentColor = getParam3f("pigmentColor",vec3f(1.f));
        const float eta          = getParamf("eta",1.4f);
        const float roughness    = getParamf("roughness",0.01f);
        // const float rcpRoughness = rcpf(roughness);

        ispcEquivalent = ispc::PathTracer_Plastic_create
          ((const ispc::vec3f&)pigmentColor,eta,roughness);
      }
    };

    OSP_REGISTER_MATERIAL(Plastic,PathTracer_Plastic);
  }
}
