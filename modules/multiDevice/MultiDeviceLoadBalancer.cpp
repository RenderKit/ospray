// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MultiDeviceLoadBalancer.h"
#include "MultiDevice.h"
#include "fb/LocalFB.h"
#include "fb/SparseFB.h"

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

  // Schedule rendering on all devices
  std::vector<std::pair<devicert::AsyncEvent, devicert::AsyncEvent>> events(
      framebuffer->objects.size());
  for (uint32_t i = 0; i < framebuffer->objects.size(); i++) {
    SparseFrameBuffer *fbi = (SparseFrameBuffer *)framebuffer->objects[i];
    Renderer *ri = (Renderer *)renderer->objects[i];
    Camera *ci = (Camera *)camera->objects[i];
    World *wi = (World *)world->objects[i];

    if (fbi->getTileIDs().size() != 0)
      events[i] = loadBalancers[i]->renderFrame(fbi, ri, ci, wi);
  }

  // Wait for all devices to finish
  for (uint32_t i = 0; i < framebuffer->objects.size(); i++) {
    events[i].first.wait();
    events[i].second.wait();

    SparseFrameBuffer *fbi = (SparseFrameBuffer *)framebuffer->objects[i];
    framebuffer->rowmajorFb->writeTiles(fbi);
  }
  framebuffer->rowmajorFb->setCompletedEvent(OSP_WORLD_RENDERED);

  // We need to call post-processing operations on fully compositioned
  // (rowmajorFb) frame buffer and wait for it to finish
  devicert::AsyncEvent event = framebuffer->rowmajorFb->postProcess();
  event.wait();

  for (uint32_t i = 0; i < framebuffer->objects.size(); i++) {
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
