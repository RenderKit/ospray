// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LoadBalancer.h"
#include "camera/Camera.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
// ispc shared
#include "fb/LocalFBShared.h"

#ifndef OSPRAY_TARGET_SYCL
#include "render/RenderTask.h"
#include "rkcommon/utility/CodeTimer.h"
#else
#include "render/RenderTaskSycl.h"
#endif

namespace ospray {

std::pair<AsyncEvent, AsyncEvent> LocalTiledLoadBalancer::renderFrame(
    FrameBuffer *fb,
    Renderer *renderer,
    Camera *camera,
    World *world,
    bool wait)
{
  fb->beginFrame();
  void *perFrameData = renderer->beginFrame(fb, world);

  // Get render tasks and process them all
  utility::ArrayView<uint32_t> taskIds =
      fb->getRenderTaskIDs(renderer->errorThreshold, renderer->spp);
  AsyncEvent rendererEvent;
  if (taskIds.size())
    rendererEvent =
        renderer->renderTasks(fb, camera, world, perFrameData, taskIds, wait);

  // Can set flag only if we wait
  if (wait)
    fb->setCompletedEvent(OSP_WORLD_RENDERED);

  // But we can queue FB post-processing kernel if any tasks has been rendered
  AsyncEvent fbEvent;
  if (taskIds.size())
    fbEvent = fb->postProcess(wait);

  // Can set flag only if we wait
  if (wait)
    fb->setCompletedEvent(OSP_FRAME_FINISHED);

  return std::make_pair(rendererEvent, fbEvent);
}

std::string LocalTiledLoadBalancer::toString() const
{
  return "ospray::LocalTiledLoadBalancer";
}

} // namespace ospray
