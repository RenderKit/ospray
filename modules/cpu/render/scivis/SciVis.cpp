// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "SciVis.h"
#include "SciVisData.h"
#include "camera/Camera.h"
#include "common/FeatureFlagsEnum.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc exports
#include "render/scivis/SciVis_ispc.h"
#else
#include "common/FeatureFlags.ih"
namespace ispc {
SYCL_EXTERNAL void SciVis_renderTask(Renderer *uniform self,
    FrameBuffer *uniform fb,
    Camera *uniform camera,
    World *uniform world,
    const uint32 *uniform taskIDs,
    const int taskIndex0,
    const uniform FeatureFlagsHandler &ffh);
}
#endif

namespace ospray {

SciVis::SciVis(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext(), device)
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

AsyncEvent SciVis::renderTasks(FrameBuffer *fb,
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
            ispc::SciVis_renderTask(&rendererSh->super,
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
  ispc::SciVis_renderTasks(
      &rendererSh->super, fbSh, cameraSh, worldSh, taskIDs.data(), numTasks);
#endif
  return event;
}

} // namespace ospray
