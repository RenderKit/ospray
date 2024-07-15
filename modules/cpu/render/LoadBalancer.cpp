// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LoadBalancer.h"
#include "camera/Camera.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
// ispc shared
#include "fb/LocalFBShared.h"

namespace ospray {

std::pair<devicert::AsyncEvent, devicert::AsyncEvent>
LocalTiledLoadBalancer::renderFrame(
    FrameBuffer *fb, Renderer *renderer, Camera *camera, World *world)
{
  fb->beginFrame();
  void *perFrameData = renderer->beginFrame(fb, world);

  // Get render tasks and process them all
  utility::ArrayView<uint32_t> taskIds =
      fb->getRenderTaskIDs(renderer->errorThreshold, renderer->spp);
  devicert::AsyncEvent rendererEvent;

  if (taskIds.size())
    rendererEvent =
        renderer->renderTasks(fb, camera, world, perFrameData, taskIds);

  // Schedule post-processing kernels
  devicert::AsyncEvent fbEvent = fb->postProcess();
  return std::make_pair(rendererEvent, fbEvent);
}

std::string LocalTiledLoadBalancer::toString() const
{
  return "ospray::LocalTiledLoadBalancer";
}

} // namespace ospray
