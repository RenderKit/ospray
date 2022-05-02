// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Glass.h"
// ispc
#include "render/materials/Glass_ispc.h"

namespace ospray {
namespace pathtracer {

Glass::Glass()
{
  getSh()->super.type = ispc::MATERIAL_TYPE_GLASS;
  getSh()->super.getBSDF = ispc::Glass_getBSDF_addr();
  getSh()->super.getTransparency = ispc::Glass_getTransparency_addr();
  getSh()->super.selectNextMedium = ispc::Glass_selectNextMedium_addr();
}

std::string Glass::toString() const
{
  return "ospray::pathtracer::Glass";
}

void Glass::commit()
{
  const float etaInside =
      getParam<float>("etaInside", getParam<float>("eta", 1.5f));

  const float etaOutside = getParam<float>("etaOutside", 1.f);

  const vec3f &attenuationColorInside =
      getParam<vec3f>("attenuationColorInside",
          getParam<vec3f>("attenuationColor", vec3f(1.f)));

  const vec3f &attenuationColorOutside =
      getParam<vec3f>("attenuationColorOutside", vec3f(1.f));

  const float attenuationDistance =
      getParam<float>("attenuationDistance", 1.0f);

  getSh()->mediumInside.ior = etaInside;
  getSh()->mediumInside.attenuation = vec3f(log(attenuationColorInside.x),
                                          log(attenuationColorInside.y),
                                          log(attenuationColorInside.z))
      / std::max(attenuationDistance, EPS);
  getSh()->mediumOutside.ior = etaOutside;
  getSh()->mediumOutside.attenuation = vec3f(log(attenuationColorOutside.x),
                                           log(attenuationColorOutside.y),
                                           log(attenuationColorOutside.z))
      / std::max(attenuationDistance, EPS);
}

} // namespace pathtracer
} // namespace ospray
