// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AORenderer.h"
#include "camera/Camera.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
#ifndef OSPRAY_TARGET_SYCL
#include "render/ao/AORenderer_ispc.h"
#else
#include "AORenderer.ih"
#endif

namespace ospray {

AORenderer::AORenderer(api::ISPCDevice &device, int defaultNumSamples)
    : AddStructShared(device.getIspcrtDevice(), device),
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

void AORenderer::renderTasks(FrameBuffer *fb,
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
    const cl::sycl::nd_range<1> dispatchRange =
        computeDispatchRange(numTasks, 16);
    cgh.parallel_for(dispatchRange, [=](cl::sycl::nd_item<1> taskIndex) {
      if (taskIndex.get_global_id(0) < numTasks) {
        ispc::AORenderer_renderTask(&rendererSh->super,
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
  ispc::AORenderer_renderTasks(&rendererSh->super,
      fbSh,
      cameraSh,
      worldSh,
      perFrameData,
      taskIDs.data(),
      numTasks);
#endif
}

} // namespace ospray
