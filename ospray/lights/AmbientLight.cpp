// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AmbientLight.h"
#include "AmbientLight_ispc.h"

namespace ospray {

AmbientLight::AmbientLight()
{
  ispcEquivalent = ispc::AmbientLight_create();
}

std::string AmbientLight::toString() const
{
  return "ospray::AmbientLight";
}

void AmbientLight::commit()
{
  Light::commit();
  ispc::AmbientLight_set(getIE());
}

} // namespace ospray
