// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Plastic.h"
// ispc
#include "render/materials/Plastic_ispc.h"

namespace ospray {
namespace pathtracer {

Plastic::Plastic()
{
  getSh()->super.type = ispc::MATERIAL_TYPE_PLASTIC;
  getSh()->super.getBSDF = ispc::Plastic_getBSDF_addr();
}

std::string Plastic::toString() const
{
  return "ospray::pathtracer::Plastic";
}

void Plastic::commit()
{
  const vec3f pigmentColor = getParam<vec3f>("pigmentColor", vec3f(1.f));
  const float eta = getParam<float>("eta", 1.4f);
  const float roughness = getParam<float>("roughness", 0.01f);

  getSh()->pigmentColor = pigmentColor;
  getSh()->eta = rcp(eta);
  getSh()->roughness = roughness;
}

} // namespace pathtracer
} // namespace ospray
