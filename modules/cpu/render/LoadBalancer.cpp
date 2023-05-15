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

  AsyncEvent rendererEvent = renderer->renderTasks(fb,
      camera,
      world,
      perFrameData,
      fb->getRenderTaskIDs(renderer->errorThreshold),
      wait);

  // Can't call renderer->endFrame() because we might still render
  if (wait) {
    renderer->endFrame(fb, perFrameData);
    fb->setCompletedEvent(OSP_WORLD_RENDERED);
  }

  // But we can queue FB post-processing kernel
  AsyncEvent fbEvent = fb->postProcess(camera, wait);

  // Can't call fb->endFrame() because we might still post-process
  if (wait) {
    fb->endFrame(renderer->errorThreshold, camera);
    fb->setCompletedEvent(OSP_FRAME_FINISHED);
  }

  return std::make_pair(rendererEvent, fbEvent);
}

std::string LocalTiledLoadBalancer::toString() const
{
  return "ospray::LocalTiledLoadBalancer";
}

} // namespace ospray
