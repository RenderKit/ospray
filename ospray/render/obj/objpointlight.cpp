#include "objpointlight.h"
#include "objpointlight_ispc.h"

namespace ospray {
  namespace obj {

    OBJPointLight::OBJPointLight() {
      ispcEquivalent = ispc::OBJPointLight_create(this);
    }

    void OBJPointLight::commit() {
      //commit inherited params
      PointLight::commit();
      //constantAttenuation
      constantAttenuation = getParam1f("attenuation.constant", 0.f);
      //linearAttenuation
      linearAttenuation = getParam1f("attenuation.linear", 0.f);
      //quadraticAttenuation
      quadraticAttenuation = getParam1f("attenuation.quadratic", 0.f);

      //PRINT(constantAttenuation);
      //PRINT(linearAttenuation);
      //PRINT(quadraticAttenuation);
      ispc::OBJPointLight_set(getIE(),
                              (ispc::vec3f&)position,
                              (ispc::vec3f&)color,
                              range,
                              constantAttenuation,
                              linearAttenuation,
                              quadraticAttenuation);
    }

    OBJPointLight::~OBJPointLight(){}

    OSP_REGISTER_LIGHT(OBJPointLight, OBJ_PointLight);

  }
}
