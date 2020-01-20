// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Light.h"
#include "Light_ispc.h"
#include "common/Util.h"

namespace ospray {

Light::Light()
{
  managedObjectType = OSP_LIGHT;
}

void Light::commit()
{
  const vec3f radiance =
      getParam<vec3f>("color", vec3f(1.f)) * getParam<float>("intensity", 1.f);
  ispc::Light_set(
      getIE(), (ispc::vec3f &)radiance, getParam<bool>("visible", true));
}

std::string Light::toString() const
{
  return "ospray::Light";
}

Light *Light::createInstance(const char *type)
{
  return createInstanceHelper<Light, OSP_LIGHT>(type);
}

OSPTYPEFOR_DEFINITION(Light *);

} // namespace ospray
