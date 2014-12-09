#include "DirectionalLight.h"
#include "DirectionalLight_ispc.h"

namespace ospray {

  DirectionalLight::DirectionalLight()
    : direction(0.f, -1.f, 0.f)
    , color(1.f, 1.f, 1.f)
  {
    ispcEquivalent = ispc::DirectionalLight_create(this);
  }

  void DirectionalLight::commit() {
    direction = getParam3f("direction", vec3f(0.f, -1.f, 0.f));
    color     = getParam3f("color", vec3f(1.f, 1.f, 1.f));

    ispc::DirectionalLight_set(getIE(), (ispc::vec3f&)color, (ispc::vec3f&)direction);
  }

}
