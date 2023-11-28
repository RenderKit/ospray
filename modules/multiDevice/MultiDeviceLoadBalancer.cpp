// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MultiDeviceLoadBalancer.h"
#include "MultiDevice.h"
#include "fb/LocalFB.h"
#include "fb/SparseFB.h"

#include <rkcommon/tasking/parallel_for.h>
#include <rkcommon/utility/ArrayView.h>

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

    if (fbi->getTileIDs().size() != 0) {
      loadBalancers[i]->renderFrame(fbi, ri, ci, wi);

      framebuffer->rowmajorFb->writeTiles(fbi);
    }
  });
  framebuffer->rowmajorFb->setCompletedEvent(OSP_WORLD_RENDERED);

  // We need to call post-processing operations on fully compositioned
  // (rowmajorFb) frame buffer and wait for it to finish
  framebuffer->rowmajorFb->postProcess(true);

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
