// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SciVisData.h"
#include "common/Instance.h"
#include "common/World.h"
#include "lights/AmbientLight.h"
#include "lights/HDRILight.h"
#include "lights/SunSkyLight.h"
// ispc shared
#include "lights/LightShared.h"

namespace ospray {

namespace {

void addVisibleOnlyToArray(std::vector<ispc::Light *> &lightShs,
    uint32_t &visibleOnly,
    ispc::Light *lightSh)
{
  if (visibleOnly == lightShs.size())
    lightShs.push_back(lightSh);
  else {
    // insert light at the visibleOnly index
    lightShs.push_back(lightShs[visibleOnly]);
    lightShs[visibleOnly] = lightSh;
  }
  visibleOnly++;
}

vec3f addLightsToArray(std::vector<ispc::Light *> &lightShs,
    uint32_t &visibleOnly,
    Ref<const DataT<Light *>> lights,
    const ispc::Instance *instanceSh)
{
  vec3f aoColor = vec3f(0.f);
  for (auto &&light : *lights) {
    // just extract color from ambient lights
    const auto ambient = dynamic_cast<const AmbientLight *>(light);
    if (ambient) {
      addVisibleOnlyToArray(
          lightShs, visibleOnly, light->createSh(0, instanceSh));
      aoColor += ambient->radiance;
      continue;
    }

    // no illumination from HDRI lights
    const auto hdri = dynamic_cast<const HDRILight *>(light);
    if (hdri) {
      addVisibleOnlyToArray(
          lightShs, visibleOnly, light->createSh(0, instanceSh));
      continue;
    }

    // sun-sky: only sun illuminates
    const auto sunsky = dynamic_cast<const SunSkyLight *>(light);
    if (sunsky) {
      addVisibleOnlyToArray(lightShs,
          visibleOnly,
          light->createSh(0, instanceSh)); // sky visible only
      lightShs.push_back(light->createSh(1, instanceSh)); // sun
      continue;
    }

    // handle the remaining types of lights
    for (uint32_t id = 0; id < light->getShCount(); id++)
      lightShs.push_back(light->createSh(id, instanceSh));
  }
  return aoColor;
}

} // namespace

SciVisData::SciVisData(const World &world)
    : AddStructShared(world.getISPCDevice().getDRTDevice())
{
  std::vector<ispc::Light *> lightShs;
  vec3f aoColor = vec3f(0.f);
  uint32_t visibleOnly = 0;

  // Add lights not assigned to any instance
  if (world.lights)
    aoColor += addLightsToArray(lightShs, visibleOnly, world.lights, nullptr);

  // Iterate through all world instances
  if (world.instances) {
    for (auto &&inst : *world.instances) {
      // Skip instances without lights
      if (!inst->group->lights)
        continue;

      // Add instance lights to array
      aoColor += addLightsToArray(
          lightShs, visibleOnly, inst->group->lights, inst->getSh());
    }
  }

  // Then create shared buffer from the temporary std::vector
  lightArray = devicert::make_buffer_shared_unique<ispc::Light *>(
      world.getISPCDevice().getDRTDevice(), lightShs);
  getSh()->lights = lightArray->sharedPtr();
  getSh()->numLights = lightArray->size();
  getSh()->numLightsVisibleOnly = visibleOnly;
  getSh()->aoColorPi = aoColor * float(pi);
}

SciVisData::~SciVisData()
{
  // Delete all lights structures
  devicert::Device &device = lightArray->getDevice();
  for (ispc::Light *lptr : *lightArray)
    device.free(lptr);
}

} // namespace ospray
