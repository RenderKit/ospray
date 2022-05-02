// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AmbientLight.h"
#include "CylinderLight.h"
#include "DirectionalLight.h"
#include "HDRILight.h"
#include "PointLight.h"
#include "QuadLight.h"
#include "SpotLight.h"
#include "SunSkyLight.h"

namespace ospray {

void registerAllLights()
{
  Light::registerType<AmbientLight>("ambient");
  Light::registerType<CylinderLight>("cylinder");
  Light::registerType<DirectionalLight>("distant");
  Light::registerType<HDRILight>("hdri");
  Light::registerType<PointLight>("sphere");
  Light::registerType<QuadLight>("quad");
  Light::registerType<SpotLight>("spot");
  Light::registerType<SunSkyLight>("sunSky");
}

} // namespace ospray
