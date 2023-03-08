// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "SciVis.h"
#include "SciVisData.h"
#include "camera/Camera.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc exports
#include "render/scivis/SciVis_ispc.h"
#else
namespace ispc {
SYCL_EXTERNAL void SciVis_renderTask(Renderer *uniform self,
    FrameBuffer *uniform fb,
    Camera *uniform camera,
    World *uniform world,
    void *uniform perFrameData,
    const uint32 *uniform taskIDs,
    const int taskIndex0);
}
#endif

namespace ospray {

SciVis::SciVis(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtDevice(), device)
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

void SciVis::renderTasks(FrameBuffer *fb,
    Camera *camera,
    World *world,
    void *perFrameData,
    const utility::ArrayView<uint32_t> &taskIDs
#ifdef OSPRAY_TARGET_SYCL
    ,
    sycl::queue &syclQueue
#endif
) const
{
  auto *rendererSh = getSh();
  auto *fbSh = fb->getSh();
  auto *cameraSh = camera->getSh();
  auto *worldSh = world->getSh();
  const size_t numTasks = taskIDs.size();

#ifdef OSPRAY_TARGET_SYCL
  const uint32_t *taskIDsPtr = taskIDs.data();
  auto event = syclQueue.submit([&](sycl::handler &cgh) {
    const sycl::nd_range<1> dispatchRange = computeDispatchRange(numTasks, 16);
    cgh.parallel_for(dispatchRange, [=](sycl::nd_item<1> taskIndex) {
      if (taskIndex.get_global_id(0) < numTasks) {
        ispc::SciVis_renderTask(&rendererSh->super,
            fbSh,
            cameraSh,
            worldSh,
            perFrameData,
            taskIDsPtr,
            taskIndex.get_global_id(0));
      }
    });
  });
  event.wait_and_throw();
  // For prints we have to flush the entire queue, because other stuff is queued
  syclQueue.wait_and_throw();
#else
  ispc::SciVis_renderTasks(&rendererSh->super,
      fbSh,
      cameraSh,
      worldSh,
      perFrameData,
      taskIDs.data(),
      numTasks);
#endif
}

} // namespace ospray
