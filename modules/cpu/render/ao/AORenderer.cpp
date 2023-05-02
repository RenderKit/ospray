// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AORenderer.h"
#include "camera/Camera.h"
#include "common/FeatureFlagsEnum.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
#ifndef OSPRAY_TARGET_SYCL
#include "render/ao/AORenderer_ispc.h"
#else
#include "AORenderer.ih"
#endif

namespace ospray {

AORenderer::AORenderer(api::ISPCDevice &device, int defaultNumSamples)
    : AddStructShared(device.getIspcrtContext(), device),
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

AsyncEvent AORenderer::renderTasks(FrameBuffer *fb,
    Camera *camera,
    World *world,
    void *,
    const utility::ArrayView<uint32_t> &taskIDs,
    bool wait) const
{
  AsyncEvent event;
  auto *rendererSh = getSh();
  auto *fbSh = fb->getSh();
  auto *cameraSh = camera->getSh();
  auto *worldSh = world->getSh();
  const size_t numTasks = taskIDs.size();

#ifdef OSPRAY_TARGET_SYCL
  const uint32_t *taskIDsPtr = taskIDs.data();
  event = device.getSyclQueue().submit([&](sycl::handler &cgh) {
    FeatureFlags ff = world->getFeatureFlags();
    ff |= featureFlags;
    ff |= fb->getFeatureFlags();
    ff |= camera->getFeatureFlags();
    cgh.set_specialization_constant<ispc::specFeatureFlags>(ff);

    const sycl::nd_range<1> dispatchRange =
        device.computeDispatchRange(numTasks, 16);
    cgh.parallel_for(dispatchRange,
        [=](sycl::nd_item<1> taskIndex, sycl::kernel_handler kh) {
          if (taskIndex.get_global_id(0) < numTasks) {
            ispc::FeatureFlagsHandler ffh(kh);
            ispc::AORenderer_renderTask(&rendererSh->super,
                fbSh,
                cameraSh,
                worldSh,
                taskIDsPtr,
                taskIndex.get_global_id(0),
                ffh);
          }
        });
  });

  if (wait)
    event.wait_and_throw();
#else
  (void)wait;
  ispc::AORenderer_renderTasks(
      &rendererSh->super, fbSh, cameraSh, worldSh, taskIDs.data(), numTasks);
#endif
  return event;
}

} // namespace ospray
