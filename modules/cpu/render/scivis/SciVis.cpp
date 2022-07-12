// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "SciVis.h"
#include "SciVisData.h"
#include "common/World.h"
// ispc exports
#include "render/scivis/SciVis_ispc.h"

namespace ospray {

SciVis::SciVis(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtDevice(), device)
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

  if (world->scivisData.get())
    return nullptr;

  // Create SciVisData object
  std::unique_ptr<SciVisData> scivisData =
      rkcommon::make_unique<SciVisData>(*world);
  world->getSh()->scivisData = scivisData->getSh();
  world->scivisData = std::move(scivisData);
  return nullptr;
}

} // namespace ospray
