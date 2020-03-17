// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Plastic.h"
// ispc
#include "Plastic_ispc.h"

namespace ospray {
namespace pathtracer {

Plastic::Plastic()
{
  ispcEquivalent = ispc::PathTracer_Plastic_create();
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

  ispc::PathTracer_Plastic_set(
      getIE(), (const ispc::vec3f &)pigmentColor, eta, roughness);
}

} // namespace pathtracer
} // namespace ospray
