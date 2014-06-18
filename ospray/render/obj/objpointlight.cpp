#include "objpointlight.h"

namespace ospray {
  namespace obj {

    void OBJPointLight::commit() {
      //commit inherited params
      PointLight::commit();
      //constantAttenuation
      constantAttenuation = getParam1f("attenuation.constant", 0.f);
      //linearAttenuation
      linearAttenuation = getParam1f("attenuation.linear", 0.f);
      //quadraticAttenuation
      quadraticAttenuation = getParam1f("attenuation.quadratic", 0.f);

      //TODO: set ispc side
    }

    OBJPointLight::~OBJPointLight(){}

    OSP_REGISTER_LIGHT(OBJPointLight, OBJ_PointLight);

  }
}
