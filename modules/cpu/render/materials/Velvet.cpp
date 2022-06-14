// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Velvet.h"
// ispc
#include "render/materials/Velvet_ispc.h"

namespace ospray {
namespace pathtracer {

Velvet::Velvet()
{
  getSh()->super.type = ispc::MATERIAL_TYPE_VELVET;
  getSh()->super.getBSDF = ispc::Velvet_getBSDF_addr();
}

std::string Velvet::toString() const
{
  return "ospray::pathtracer::Velvet";
}

void Velvet::commit()
{
  vec3f reflectance = getParam<vec3f>("reflectance", vec3f(.4f, 0.f, 0.f));
  float backScattering = getParam<float>("backScattering", .5f);
  vec3f horizonScatteringColor =
      getParam<vec3f>("horizonScatteringColor", vec3f(.75f, .1f, .1f));
  float horizonScatteringFallOff =
      getParam<float>("horizonScatteringFallOff", 10);

  getSh()->reflectance = reflectance;
  getSh()->backScattering = backScattering;
  getSh()->horizonScatteringColor = horizonScatteringColor;
  getSh()->horizonScatteringFallOff = horizonScatteringFallOff;
}

} // namespace pathtracer
} // namespace ospray
