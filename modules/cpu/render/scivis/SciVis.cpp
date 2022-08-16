// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "SciVis.h"
#include "SciVisData.h"
#include "camera/Camera.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
#ifndef OSPRAY_TARGET_DPCPP
// ispc exports
#include "render/scivis/SciVis_ispc.h"
#else
#include "SciVis.ih"
#include "render/RendererRenderTaskFn.inl"
#endif

namespace ospray {

SciVis::SciVis(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtDevice(), device)
{
#ifndef OSPRAY_TARGET_DPCPP
  getSh()->super.renderSample =
      reinterpret_cast<ispc::Renderer_RenderSampleFct>(
          ispc::SciVis_renderSample_addr());
#endif
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
  getSh()->aoSamples = getParam<int>("aoSamples", 1);
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

#ifdef OSPRAY_TARGET_DPCPP
void SciVis::renderTasks(FrameBuffer *fb,
    Camera *camera,
    World *world,
    void *perFrameData,
    const utility::ArrayView<uint32_t> &taskIDs,
    sycl::queue &syclQueue) const
{
  auto *rendererSh = getSh();
  auto *fbSh = fb->getSh();
  auto *cameraSh = camera->getSh();
  auto *worldSh = world->getSh();
  const uint32_t *taskIDsPtr = taskIDs.data();
  const size_t numTasks = taskIDs.size();

  auto event = syclQueue.submit([&](sycl::handler &cgh) {
    const cl::sycl::nd_range<1> dispatchRange =
        computeDispatchRange(numTasks, RTC_SYCL_SIMD_WIDTH);
    cgh.parallel_for(
        dispatchRange, [=](cl::sycl::nd_item<1> taskIndex) RTC_SYCL_KERNEL {
          if (taskIndex.get_global_id(0) < numTasks) {
            ispc::Renderer_default_renderTask(&rendererSh->super,
                fbSh,
                cameraSh,
                worldSh,
                perFrameData,
                taskIDsPtr,
                taskIndex.get_global_id(0),
                ispc::SciVis_renderSample);
          }
        });
  });
  event.wait_and_throw();
  // For prints we have to flush the entire queue, because other stuff is queued
  syclQueue.wait_and_throw();
}

/*
void SciVis::setGPUFunctionPtrs(sycl::queue &syclQueue)
{
  Renderer::setGPUFunctionPtrs(syclQueue);

  auto *sSh = getSh();
  auto event = syclQueue.submit([&](sycl::handler &cgh) {
    cgh.parallel_for(1, [=](cl::sycl::id<1>) RTC_SYCL_KERNEL {
      sSh->super.renderSample = ispc::SciVis_renderSample;
    });
  });
  event.wait();
}
*/
#endif

} // namespace ospray
