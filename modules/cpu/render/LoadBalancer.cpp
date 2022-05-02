// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LoadBalancer.h"
#include "Renderer.h"
#include "api/Device.h"
#include "rkcommon/tasking/parallel_for.h"

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
  if (renderer->errorThreshold > 0.f) {
    std::vector<uint32_t> activeTasks;
    for (auto &i : renderTaskIDs) {
      const float error = fb->taskError(i);
      if (error > renderer->errorThreshold) {
        activeTasks.push_back(i);
      }
    }
    renderer->renderTasks(fb,
        camera,
        world,
        perFrameData,
        utility::ArrayView<uint32_t>(activeTasks));
  } else {
    renderer->renderTasks(fb, camera, world, perFrameData, renderTaskIDs);
  }
}

std::string LocalTiledLoadBalancer::toString() const
{
  return "ospray::LocalTiledLoadBalancer";
}

} // namespace ospray
