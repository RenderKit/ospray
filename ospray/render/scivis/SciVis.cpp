// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "SciVis.h"
#include "lights/AmbientLight.h"
#include "lights/HDRILight.h"
// ispc exports
#include "common/World_ispc.h"
#include "render/scivis/SciVis_ispc.h"

namespace ospray {

SciVis::SciVis(int defaultNumSamples, bool defaultShadowsEnabled)
    : aoSamples(defaultNumSamples), shadowsEnabled(defaultShadowsEnabled)
{
  ispcEquivalent = ispc::SciVis_create(this);
}

std::string SciVis::toString() const
{
  return "ospray::render::SciVis";
}

void SciVis::commit()
{
  Renderer::commit();

  ispc::SciVis_set(getIE(),
      getParam<bool>("shadows", shadowsEnabled),
      getParam<int>("aoSamples", aoSamples),
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

  if (world->lights) {
    for (auto &&light : *world->lights) {
      // extract color from ambient lights and remove them
      const AmbientLight *const ambient =
          dynamic_cast<const AmbientLight *>(light);
      if (ambient) {
        aoColor += ambient->radiance;
      } else {
        // also ignore HDRI lights TODO but put in background
        if (!dynamic_cast<const HDRILight *>(light)) {
          lightArray.push_back(light->getIE());
          if (light->getSecondIE().has_value())
            lightArray.push_back(light->getSecondIE().value());
        }
      }
    }
  }

  void **lightPtr = lightArray.empty() ? nullptr : &lightArray[0];
  ispc::World_setSciVisData(
      world->getIE(), (ispc::vec3f &)aoColor, lightPtr, lightArray.size());

  world->scivisDataValid = true;

  return nullptr;
}

} // namespace ospray
