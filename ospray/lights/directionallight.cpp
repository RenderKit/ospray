#include "directionallight.h"

namespace ospray {

  DirectionalLight::DirectionalLight()
    : direction(0.f, -1.f, 0.f)
    , color(1.f, 1.f, 1.f)
  {}

  void DirectionalLight::commit() {
    direction = getParam3f("direction", vec3f(0.f, -1.f, 0.f));
    color     = getParam3f("color", vec3f(1.f, 1.f, 1.f));
  }

  OSP_REGISTER_LIGHT(DirectionalLight, DirectionalLight);

}
