// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "SciVis.h"
#include "lights/AmbientLight.h"
#include "lights/HDRILight.h"
#include "lights/SunSkyLight.h"
// ispc exports
#include "common/World_ispc.h"
#include "render/scivis/SciVis_ispc.h"

namespace ospray {

namespace {

void addVisibleOnlyToArray(
    std::vector<void *> &lightIEs, uint32_t &visibleOnly, void *lightIE)
{
  if (visibleOnly == lightIEs.size())
    lightIEs.push_back(lightIE);
  else {
    lightIEs.push_back(lightIEs[visibleOnly]);
    lightIEs[visibleOnly] = lightIE;
  }
  visibleOnly++;
}

vec3f addLightsToArray(std::vector<void *> &lightIEs,
    uint32_t &visibleOnly,
    Ref<const DataT<Light *>> &lights,
    const void *instanceIE)
{
  vec3f aoColor = vec3f(0.f);
  for (auto &&light : *lights) {
    // create ISPC equivalent for the light
    void *lightIE = light->createIE(instanceIE);

    // just extract color from ambient lights
    const auto ambient = dynamic_cast<const AmbientLight *>(light);
    if (ambient) {
      addVisibleOnlyToArray(lightIEs, visibleOnly, lightIE);
      aoColor += ambient->radiance;
      continue;
    }

    // no illumination from HDRI lights
    const auto hdri = dynamic_cast<const HDRILight *>(light);
    if (hdri) {
      addVisibleOnlyToArray(lightIEs, visibleOnly, lightIE);
      continue;
    }

    // sun-sky: only sun illuminates
    const auto sunsky = dynamic_cast<const SunSkyLight *>(light);
    if (sunsky) {
      // just sky visible
      addVisibleOnlyToArray(lightIEs, visibleOnly, lightIE);
      lightIEs.push_back(light->createSecondIE(instanceIE)); // sun
      continue;
    }

    // handle the remaining types of lights
    lightIEs.push_back(lightIE);
  }
  return aoColor;
}

} // namespace

SciVis::SciVis()
{
  ispcEquivalent = ispc::SciVis_create();
}

std::string SciVis::toString() const
{
  return "ospray::render::SciVis";
}

void SciVis::commit()
{
  Renderer::commit();

  ispc::SciVis_set(getIE(),
      getParam<bool>("shadows", false),
      getParam<bool>("visibleLights", false),
      getParam<int>("aoSamples", 0),
      getParam<float>("aoDistance", getParam<float>("aoRadius", 1e20f)),
      getParam<float>("volumeSamplingRate", 1.f));
}

void *SciVis::beginFrame(FrameBuffer *, World *world)
{
  if (!world)
    return nullptr;

  if (world->scivisDataValid)
    return nullptr;

  std::vector<void *> lightArray;
  vec3f aoColor = vec3f(0.f);
  uint32_t visibleOnly = 0;

  // Add lights not assigned to any instance
  if (world->lights)
    aoColor +=
        addLightsToArray(lightArray, visibleOnly, world->lights, nullptr);

  // Iterate through all world instances
  if (world->instances) {
    for (auto &&inst : *world->instances) {
      // Skip instances without lights
      if (!inst->group->lights)
        continue;

      // Add instance lights to array
      aoColor += addLightsToArray(
          lightArray, visibleOnly, inst->group->lights, inst->getIE());
    }
  }

  ispc::World_setSciVisData(world->getIE(),
      (ispc::vec3f &)aoColor,
      lightArray.empty() ? nullptr : lightArray.data(),
      lightArray.size(),
      visibleOnly);

  world->scivisDataValid = true;
  return nullptr;
}

} // namespace ospray
