// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ThinGlass.h"
// ispc
#include "render/materials/ThinGlass_ispc.h"

namespace ospray {
namespace pathtracer {

ThinGlass::ThinGlass()
{
  getSh()->super.type = ispc::MATERIAL_TYPE_THINGLASS;
  getSh()->super.getBSDF = ispc::ThinGlass_getBSDF_addr();
  getSh()->super.getTransparency = ispc::ThinGlass_getTransparency_addr();
}

std::string ThinGlass::toString() const
{
  return "ospray::pathtracer::ThinGlass";
}

void ThinGlass::commit()
{
  const float eta = getParam<float>("eta", 1.5f);
  MaterialParam3f attenuationColor =
      getMaterialParam3f("attenuationColor", vec3f(1.f));
  const float attenuationDistance = getParam<float>("attenuationDistance", 1.f);
  const float thickness = getParam<float>("thickness", 1.f);

  getSh()->eta = rcp(eta);
  getSh()->attenuationScale =
      thickness * rcp(std::max(attenuationDistance, EPS));
  const vec3f &acf = attenuationColor.factor;
  getSh()->attenuation =
      vec3f(log(acf.x), log(acf.y), log(acf.z)) * getSh()->attenuationScale;
  getSh()->attenuationColorMap = attenuationColor.tex;
}

} // namespace pathtracer
} // namespace ospray
