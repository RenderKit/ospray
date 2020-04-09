// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "QuadLight.h"
#include "QuadLight_ispc.h"

namespace ospray {

QuadLight::QuadLight()
{
  ispcEquivalent = ispc::QuadLight_create();
}

std::string QuadLight::toString() const
{
  return "ospray::QuadLight";
}

void QuadLight::commit()
{
  Light::commit();
  position = getParam<vec3f>("position", vec3f(0.f));
  edge1 = getParam<vec3f>("edge1", vec3f(1.f, 0.f, 0.f));
  edge2 = getParam<vec3f>("edge2", vec3f(0.f, 1.f, 0.f));

  ispc::QuadLight_set(getIE(),
      (ispc::vec3f &)position,
      (ispc::vec3f &)edge1,
      (ispc::vec3f &)edge2);
}

} // namespace ospray
