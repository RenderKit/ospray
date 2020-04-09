// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Luminous.h"
// ispc
#include "Luminous_ispc.h"

namespace ospray {
namespace pathtracer {

Luminous::Luminous()
{
  ispcEquivalent = ispc::PathTracer_Luminous_create();
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

  ispc::PathTracer_Luminous_set(
      getIE(), (const ispc::vec3f &)radiance, transparency);
}

} // namespace pathtracer
} // namespace ospray
