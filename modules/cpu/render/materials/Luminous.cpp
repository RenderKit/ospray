// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Luminous.h"
// ispc
#include "render/materials/Luminous_ispc.h"

namespace ospray {
namespace pathtracer {

Luminous::Luminous()
{
  getSh()->super.type = ispc::MATERIAL_TYPE_LUMINOUS;
  getSh()->super.getBSDF = ispc::Luminous_getBSDF_addr();
  getSh()->super.getTransparency = ispc::Luminous_getTransparency_addr();
}

std::string Luminous::toString() const
{
  return "ospray::pathtracer::Luminous";
}

void Luminous::commit()
{
  const vec3f radiance =
      getParam<vec3f>("color", vec3f(1.f)) * getParam<float>("intensity", 1.f);
  const float transparency = getParam<float>("transparency", 0.f);

  getSh()->super.emission = radiance;
  getSh()->transparency = transparency;
}

} // namespace pathtracer
} // namespace ospray
