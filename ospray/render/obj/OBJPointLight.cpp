#include "OBJPointLight.h"
#include "OBJPointLight_ispc.h"

namespace ospray {
  namespace obj {

    //! Create a new OBJPointLight object and ispc equivalent
    OBJPointLight::OBJPointLight() {
      ispcEquivalent = ispc::OBJPointLight_create(this);
    }

    //! Commit parameters to class members. Pass data on to ispc side object.
    void OBJPointLight::commit() {

      //commit inherited params
      PointLight::commit();

      constantAttenuation = getParam1f("attenuation.constant", 0.f);
      linearAttenuation = getParam1f("attenuation.linear", 0.f);
      quadraticAttenuation = getParam1f("attenuation.quadratic", 0.f);

      ispc::OBJPointLight_set(getIE(),
                              (ispc::vec3f&)position,
                              (ispc::vec3f&)color,
                              range,
                              constantAttenuation,
                              linearAttenuation,
                              quadraticAttenuation);
    }

    //! Destroy an OBJPointLight object
    OBJPointLight::~OBJPointLight(){}

    //Register the light type
    OSP_REGISTER_LIGHT(OBJPointLight, OBJ_PointLight);

  } // ::ospray::obj
} // ::ospray
