// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MultiDeviceLoadBalancer.h"
#include "MultiDevice.h"
#include "fb/LocalFB.h"
#include "fb/SparseFB.h"

#include <rkcommon/tasking/parallel_for.h>

namespace ospray {

MultiDeviceLoadBalancer::MultiDeviceLoadBalancer(
    const std::vector<std::shared_ptr<TiledLoadBalancer>> &loadBalancers)
    : loadBalancers(loadBalancers)

{}

void MultiDeviceLoadBalancer::renderFrame(
    api::MultiDeviceFrameBuffer *framebuffer,
    api::MultiDeviceObject *renderer,
    api::MultiDeviceObject *camera,
    api::MultiDeviceObject *world)
{
  framebuffer->rowmajorFb->beginFrame();

  tasking::parallel_for(framebuffer->objects.size(), [&](size_t i) {
    SparseFrameBuffer *fbi = (SparseFrameBuffer *)framebuffer->objects[i];
    Renderer *ri = (Renderer *)renderer->objects[i];
    Camera *ci = (Camera *)camera->objects[i];
    World *wi = (World *)world->objects[i];

    if (!fbi->getTileIDs().empty()) {
      loadBalancers[i]->renderFrame(fbi, ri, ci, wi);

      framebuffer->rowmajorFb->writeTiles(fbi);
    }
  });
  framebuffer->rowmajorFb->setCompletedEvent(OSP_WORLD_RENDERED);

  // TODO: These might not be accessible to the local fb on the CPU,
  // so the multidevice needs to kind of manage "a bit" of a CPU device?
  // Also here, for executing frame ops, the rowmajorFb doesn't get parameters
  // set on it. It either needs params to be set on it (and then params from
  // which subdevice?) or borrow them here from a subdevice, which also doesn't
  // fit well
  Renderer *r0 = (Renderer *)renderer->objects[0];
  Camera *c0 = (Camera *)camera->objects[0];
  framebuffer->rowmajorFb->endFrame(r0->errorThreshold, c0);

  // Now copy the task error data back into the sparse fb's task error buffer,
  // since the rowmajorFb has done the task error region refinement on the full
  // framebuffer
  if (framebuffer->rowmajorFb->hasVarianceBuf()) {
    tasking::parallel_for(framebuffer->objects.size(), [&](size_t i) {
      SparseFrameBuffer *fbi = (SparseFrameBuffer *)framebuffer->objects[i];
      const vec2i totalRenderTasks =
          framebuffer->rowmajorFb->getNumRenderTasks();
      const vec2i renderTaskSize = fbi->getRenderTaskSize();
      const auto &tiles = fbi->getTiles();

      uint32_t renderTaskID = 0;
      for (size_t tid = 0; tid < tiles.size(); ++tid) {
        const auto &tile = tiles[tid];
        const box2i taskRegion(tile.region.lower / renderTaskSize,
            tile.region.upper / renderTaskSize);
        for (int y = taskRegion.lower.y; y < taskRegion.upper.y; ++y) {
          for (int x = taskRegion.lower.x; x < taskRegion.upper.x;
               ++x, ++renderTaskID) {
            const vec2i task(x, y);
            float error = framebuffer->rowmajorFb->taskError(
                task.x + task.y * totalRenderTasks.x);
            fbi->setTaskError(renderTaskID, error);
          }
        }
      }
    });
  }

  for (size_t i = 0; i < framebuffer->objects.size(); ++i) {
    SparseFrameBuffer *fbi = (SparseFrameBuffer *)framebuffer->objects[i];
    fbi->setCompletedEvent(OSP_FRAME_FINISHED);
  }
  framebuffer->rowmajorFb->setCompletedEvent(OSP_FRAME_FINISHED);
}

std::string MultiDeviceLoadBalancer::toString() const
{
  return "ospray::MultiDeviceLoadBalancer";
}
} // namespace ospray
