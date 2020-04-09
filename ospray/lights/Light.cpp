// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Light.h"
#include "Light_ispc.h"
#include "common/Util.h"

namespace ospray {

static FactoryMap<Light> g_lightsMap;

// Light definitions //////////////////////////////////////////////////////////

Light::Light()
{
  managedObjectType = OSP_LIGHT;
}

void Light::commit()
{
  radiance =
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
  return createInstanceHelper(type, g_lightsMap[type]);
}

void Light::registerType(const char *type, FactoryFcn<Light> f)
{
  g_lightsMap[type] = f;
}

utility::Optional<void *> Light::getSecondIE()
{
  utility::Optional<void *> emptyOptional;
  return emptyOptional;
}

OSPTYPEFOR_DEFINITION(Light *);

} // namespace ospray
