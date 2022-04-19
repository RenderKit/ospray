// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AORenderer.h"
#include "camera/Camera.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
#ifndef OSPRAY_TARGET_DPCPP
#include "render/ao/AORenderer_ispc.h"
#else
#include "AORenderer.ih"
#include "render/RendererRenderTaskFn.inl"
#endif

namespace ospray {

AORenderer::AORenderer(api::ISPCDevice &device, int defaultNumSamples)
    : AddStructShared(device.getIspcrtDevice(), device),
      aoSamples(defaultNumSamples)
{
#ifndef OSPRAY_TARGET_DPCPP
  getSh()->super.renderSample =
      reinterpret_cast<ispc::Renderer_RenderSampleFct>(
          ispc::AORenderer_renderSample_addr());
#endif
}

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

#ifdef OSPRAY_TARGET_DPCPP
void AORenderer::renderTasks(FrameBuffer *fb,
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
                ispc::AORenderer_renderSample);
          }
        });
  });
  event.wait_and_throw();
  // For prints we have to flush the entire queue, because other stuff is queued
  syclQueue.wait_and_throw();
}

/*
void AORenderer::setGPUFunctionPtrs(sycl::queue &syclQueue)
{
  Renderer::setGPUFunctionPtrs(syclQueue);

  auto *sSh = getSh();
  auto event = syclQueue.submit([&](sycl::handler &cgh) {
    cgh.parallel_for(1, [=](cl::sycl::id<1>) RTC_SYCL_KERNEL {
      sSh->super.renderSample = ispc::AORenderer_renderSample;
    });
  });
  event.wait();
}
*/
#endif

} // namespace ospray
