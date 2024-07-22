// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AORenderer.h"
#include "camera/Camera.h"
#include "common/DeviceRT.h"
#include "common/FeatureFlagsEnum.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"

// Kernel launcher is defined in another compilation unit
DECLARE_RENDERER_KERNEL_LAUNCHER(AORenderer_renderTaskLauncher);

namespace ospray {

AORenderer::AORenderer(api::ISPCDevice &device, int defaultNumSamples)
    : AddStructShared(device.getDRTDevice(), device),
      aoSamples(defaultNumSamples)
{}

std::string AORenderer::toString() const
{
  return "ospray::render::AO";
}

void AORenderer::commit()
{
  Renderer::commit();

  getSh()->aoSamples = getParam<int>("aoSamples", aoSamples);
  getSh()->aoRadius =
      getParam<float>("aoDistance", getParam<float>("aoRadius", 1e20f));
  getSh()->aoIntensity = getParam<float>("aoIntensity", 1.f);
  getSh()->volumeSamplingRate = getParam<float>("volumeSamplingRate", 1.f);
}

devicert::AsyncEvent AORenderer::renderTasks(FrameBuffer *fb,
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
      ispc::AORenderer_renderTaskLauncher,
      rendererSh,
      fbSh,
      cameraSh,
      worldSh,
      taskIDs.data(),
      ff);
}

} // namespace ospray
