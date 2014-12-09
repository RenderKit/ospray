#include "ospray/lights/DirectionalLight.h"
#include "ospray/lights/PointLight.h"
#include "ospray/lights/SpotLight.h"

namespace ospray {

  //! A directional light.
  OSP_REGISTER_LIGHT(DirectionalLight, DirectionalLight);

  //! A point light with bounded range.
  OSP_REGISTER_LIGHT(PointLight, PointLight);

  //! A spot light with bounded angle and range.
  OSP_REGISTER_LIGHT(SpotLight, SpotLight);

} // namespace ospray

