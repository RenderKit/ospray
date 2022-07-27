// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "SciVis.h"
#include "common/Instance.h"
#include "common/World.h"
#include "lights/AmbientLight.h"
#include "lights/HDRILight.h"
#include "lights/SunSkyLight.h"
// ispc exports
#include "render/scivis/SciVis_ispc.h"

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
    Ref<const DataT<Light *>> &lights,
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

SciVis::SciVis()
{
  getSh()->super.renderSample = ispc::SciVis_renderSample_addr();
}

std::string SciVis::toString() const
{
  return "ospray::render::SciVis";
}

void SciVis::commit()
{
  Renderer::commit();

  getSh()->shadowsEnabled = getParam<bool>("shadows", false);
  getSh()->visibleLights = getParam<bool>("visibleLights", false);
  getSh()->aoSamples = getParam<int>("aoSamples", 0);
  getSh()->aoRadius =
      getParam<float>("aoDistance", getParam<float>("aoRadius", 1e20f));
  getSh()->volumeSamplingRate = getParam<float>("volumeSamplingRate", 1.f);
}

void *SciVis::beginFrame(FrameBuffer *, World *world)
{
  if (!world)
    return nullptr;

  if (world->scivisDataValid)
    return nullptr;

  std::vector<ispc::Light *> lightArray;
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
          lightArray, visibleOnly, inst->group->lights, inst->getSh());
    }
  }

  // Prepare scivis data structure
  ispc::SciVisData &sd = world->getSh()->scivisData;
  sd.destroy();
  sd.create(lightArray.data(), lightArray.size(), visibleOnly, aoColor);
  world->scivisDataValid = true;

  return nullptr;
}

} // namespace ospray
