// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LoadBalancer.h"
#include "Renderer.h"
#include "api/Device.h"
#include "common/Group.h"
#include "common/Instance.h"
#include "rkcommon/tasking/parallel_for.h"

#include "fb/LocalFBShared.h"

namespace ospray {

void LocalTiledLoadBalancer::renderFrame(
    FrameBuffer *fb, Renderer *renderer, Camera *camera, World *world)
{
  fb->beginFrame();
  void *perFrameData = renderer->beginFrame(fb, world);

  runRenderTasks(
      fb, renderer, camera, world, fb->getRenderTaskIDs(), perFrameData);

  renderer->endFrame(fb, perFrameData);

  fb->setCompletedEvent(OSP_WORLD_RENDERED);
  fb->endFrame(renderer->errorThreshold, camera);
  fb->setCompletedEvent(OSP_FRAME_FINISHED);
}

void LocalTiledLoadBalancer::runRenderTasks(FrameBuffer *fb,
    Renderer *renderer,
    Camera *camera,
    World *world,
    const utility::ArrayView<uint32_t> &renderTaskIDs,
    void *perFrameData)
{
#if 0
  if (renderer->errorThreshold > 0.f) {
    std::vector<uint32_t> activeTasks;
    for (auto &i : renderTaskIDs) {
      const float error = fb->taskError(i);
      if (error > renderer->errorThreshold) {
        activeTasks.push_back(i);
      }
    }

#ifdef OSPRAY_TARGET_DPCPP
    // TODO: active tasks must be a buffer shared
    const sycl::range<1> dispatchSize = activeTasks.size();
    auto *rendererSh = renderer->getSh();
    auto *fbSh = fb->getSh();
    auto *cameraSh = camera->getSh();
    auto *worldSh = world->getSh();
    const uint32_t *taskIDs = renderTaskIDs.data();
    auto event = syclQueue.submit([&](sycl::handler &cgh) {
      cgh.parallel_for(dispatchSize, [=](cl::sycl::id<1> taskIndex) {
        rendererSh->renderTask(rendererSh,
            fbSh,
            cameraSh,
            worldSh,
            perFrameData,
            taskIDs,
            taskIndex);
      });
    });
    event.wait();
#else
    renderer->renderTasks(fb,
        camera,
        world,
        perFrameData,
        utility::ArrayView<uint32_t>(activeTasks));
#endif
  } else {
#endif
  renderer->renderTasks(fb,
      camera,
      world,
      perFrameData,
      renderTaskIDs
#ifdef OSPRAY_TARGET_DPCPP
      ,
      *syclQueue
#endif
  );
#if 0
  }
#endif
  std::cout << std::flush;
}

#ifdef OSPRAY_TARGET_DPCPP
void LocalTiledLoadBalancer::setQueue(sycl::queue *sq)
{
  syclQueue = sq;
}
#endif

std::string LocalTiledLoadBalancer::toString() const
{
  return "ospray::LocalTiledLoadBalancer";
}

} // namespace ospray
