// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "SciVis.h"
#include "SciVisData.h"
#include "camera/Camera.h"
#include "common/DeviceRT.h"
#include "common/FeatureFlagsEnum.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"

// Kernel launcher is defined in another compilation unit
DECLARE_RENDERER_KERNEL_LAUNCHER(SciVis_renderTaskLauncher);

namespace ospray {

SciVis::SciVis(api::ISPCDevice &device)
    : AddStructShared(device.getDRTDevice(), device)
{}

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
  getSh()->aoRadius = getParam<float>("aoDistance", 1e20f);
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

devicert::AsyncEvent SciVis::renderTasks(FrameBuffer *fb,
    Camera *camera,
    World *world,
    void *,
    const utility::ArrayView<uint32_t> &taskIDs) const
{
  // Gather feature flags from all components
  FeatureFlags ff = world->getFeatureFlags();
  ff |= featureFlags;
  ff |= fb->getFeatureFlags();
  ff |= camera->getFeatureFlags();

  // Prepare parameters for kernel launch
  auto *rendererSh = &getSh()->super;
  auto *fbSh = fb->getSh();
  auto *cameraSh = camera->getSh();
  auto *worldSh = world->getSh();
  const size_t numTasks = taskIDs.size();
  const vec2i taskSize = fb->getRenderTaskSize();
  const vec3ui itemDims = vec3ui(taskSize.x, taskSize.y, numTasks);

  // Launch rendering kernel on the device
  return drtDevice.launchRendererKernel(itemDims,
      ispc::SciVis_renderTaskLauncher,
      rendererSh,
      fbSh,
      cameraSh,
      worldSh,
      taskIDs.data(),
      ff);
}

} // namespace ospray
