// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Alloy.h"
#include "common/Data.h"
#include "math/spectrum.h"
// ispc
#include "render/pathtracer/materials/Alloy_ispc.h"

namespace ospray {
namespace pathtracer {

Alloy::Alloy()
{
  ispcEquivalent = ispc::PathTracer_Alloy_create();
}

std::string Alloy::toString() const
{
  return "ospray::pathtracer::Alloy";
}

//! \brief commit the material's parameters
void Alloy::commit()
{
  MaterialParam3f color = getMaterialParam3f("color", vec3f(.9f));
  MaterialParam3f edgeColor = getMaterialParam3f("edgeColor", vec3f(1.f));
  MaterialParam1f roughness = getMaterialParam1f("roughness", .1f);

  ispc::PathTracer_Alloy_set(getIE(),
      (const ispc::vec3f &)color.factor,
      color.tex,
      (const ispc::vec3f &)edgeColor.factor,
      edgeColor.tex,
      roughness.factor,
      roughness.tex);
}

} // namespace pathtracer
} // namespace ospray
