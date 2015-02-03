#include "ospray/common/Material.h"
#include "Matte_ispc.h"

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
