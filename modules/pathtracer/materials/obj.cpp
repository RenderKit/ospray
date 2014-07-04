#include "ospray/common/material.h"
#include "obj_ispc.h"

namespace ospray {
  namespace pathtracer {
    struct OBJMaterial : public ospray::Material {
      //! \brief common function to help printf-debugging 
      /*! Every derived class should overrride this! */
      virtual std::string toString() const { return "ospray::pathtracer::OBJMaterial"; }
      
      //! \brief commit the material's parameters
      virtual void commit() {
        if (getIE() == NULL) {
          void* map_d = NULL;
          const float d = getParam1f("d",getParam1f("alpha",1.f));
          void* map_Kd = NULL;
          const vec3f Kd = getParam3f("Kd",getParam3f("kd",getParam3f("color",vec3f(1.f))));
          void* map_Ks = NULL;
          const vec3f Ks = vec3f(1.f);
          void* map_Ns = NULL;
          const float Ns = 1.f;
          void* map_Bump = NULL;
          ispcEquivalent = ispc::PathTracer_OBJ_create
            (map_d,d,
             map_Kd,(const ispc::vec3f&)Kd,
             map_Ks,(const ispc::vec3f&)Ks,
             map_Ns,Ns,
             map_Bump);
        }
      }
    };

    OSP_REGISTER_MATERIAL(OBJMaterial,PathTracer_OBJMaterial);
  }
}
