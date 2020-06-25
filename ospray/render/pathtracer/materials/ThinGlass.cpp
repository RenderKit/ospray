// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ThinGlass.h"
// ispc
#include "render/pathtracer/materials/ThinGlass_ispc.h"

namespace ospray {
namespace pathtracer {

ThinGlass::ThinGlass()
{
  ispcEquivalent = ispc::PathTracer_ThinGlass_create();
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

  ispc::PathTracer_ThinGlass_set(getIE(),
      eta,
      (const ispc::vec3f &)attenuationColor,
      attenuationColor.tex,
      attenuationDistance,
      thickness);
}

} // namespace pathtracer
} // namespace ospray
