// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Velvet.h"
// ispc
#include "Velvet_ispc.h"

namespace ospray {
namespace pathtracer {

std::string Velvet::toString() const
{
  return "ospray::pathtracer::Velvet";
}

Velvet::Velvet()
{
  ispcEquivalent = ispc::PathTracer_Velvet_create();
}

void Velvet::commit()
{
  vec3f reflectance = getParam<vec3f>("reflectance", vec3f(.4f, 0.f, 0.f));
  float backScattering = getParam<float>("backScattering", .5f);
  vec3f horizonScatteringColor =
      getParam<vec3f>("horizonScatteringColor", vec3f(.75f, .1f, .1f));
  float horizonScatteringFallOff =
      getParam<float>("horizonScatteringFallOff", 10);

  ispc::PathTracer_Velvet_set(getIE(),
      (const ispc::vec3f &)reflectance,
      (const ispc::vec3f &)horizonScatteringColor,
      horizonScatteringFallOff,
      backScattering);
}

} // namespace pathtracer
} // namespace ospray
