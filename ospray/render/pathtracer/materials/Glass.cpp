// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Glass.h"
// ispc
#include "Glass_ispc.h"

namespace ospray {
namespace pathtracer {

Glass::Glass()
{
  ispcEquivalent = ispc::PathTracer_Glass_create();
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

  ispc::PathTracer_Glass_set(ispcEquivalent,
      etaInside,
      (const ispc::vec3f &)attenuationColorInside,
      etaOutside,
      (const ispc::vec3f &)attenuationColorOutside,
      attenuationDistance);
}

} // namespace pathtracer
} // namespace ospray
