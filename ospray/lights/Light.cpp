// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Light.h"
#include "common/Util.h"
#include "lights/Light_ispc.h"

namespace ospray {

static FactoryMap<Light> g_lightsMap;

// Light definitions //////////////////////////////////////////////////////////

Light::Light()
{
  managedObjectType = OSP_LIGHT;
}

void Light::commit()
{
  coloredIntensity =
      getParam<vec3f>("color", vec3f(1.f)) * getParam<float>("intensity", 1.f);
  ispc::Light_set(getIE(), getParam<bool>("visible", true));
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

void Light::queryIntensityQuantityType(const OSPIntensityQuantity &defaultIQ)
{
  intensityQuantity =
      (OSPIntensityQuantity)getParam<uint8_t>("intensityQuantity", defaultIQ);
}

utility::Optional<void *> Light::getSecondIE()
{
  utility::Optional<void *> emptyOptional;
  return emptyOptional;
}

OSPTYPEFOR_DEFINITION(Light *);

} // namespace ospray
