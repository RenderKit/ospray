// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LoadBalancer.h"
#include "Renderer.h"
#include "api/Device.h"
#include "common/BufferShared.h"
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
  // We'll only have error data to check against the renderer's threshold if the
  // framebuffer has a variance channel, if it doesn't we'll just end up
  // rendering all tasks anyway.
  if (renderer->errorThreshold > 0.f && fb->hasVarianceBuf()) {
    std::vector<uint32_t> activeTasks;
    for (auto &i : renderTaskIDs) {
      const float error = fb->taskError(i);
      if (error > renderer->errorThreshold) {
        activeTasks.push_back(i);
      }
    }

    // We need the active tasks data in USM, so create a buffer shared here
    // TODO later on we could write this directly above by making the active
    // tasks vector in USM instead of copying it here
    BufferShared<uint32_t> activeTasksShared(
        fb->getISPCDevice().getIspcrtDevice(), activeTasks);

    renderer->renderTasks(fb,
        camera,
        world,
        perFrameData,
        utility::ArrayView<uint32_t>(
            activeTasksShared.data(), activeTasksShared.size())
#ifdef OSPRAY_TARGET_SYCL
            ,
        *syclQueue
#endif
    );
  } else {
    renderer->renderTasks(fb,
        camera,
        world,
        perFrameData,
        renderTaskIDs
#ifdef OSPRAY_TARGET_SYCL
        ,
        *syclQueue
#endif
    );
  }
}

#ifdef OSPRAY_TARGET_SYCL
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
