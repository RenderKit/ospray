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

Renderer::Event LocalTiledLoadBalancer::renderFrame(FrameBuffer *fb,
    Renderer *renderer,
    Camera *camera,
    World *world,
    bool wait)
{
  fb->beginFrame();
  void *perFrameData = renderer->beginFrame(fb, world);

  Renderer::Event event = renderer->renderTasks(fb,
      camera,
      world,
      perFrameData,
      fb->getRenderTaskIDs(renderer->errorThreshold),
      wait);

  // No renderer->endFrame() and fb->endFrame() on GPU.
  // Frame post-processing need to be done as a separate
  // kernel submitted to the main compute queue.
  if (wait) {
    renderer->endFrame(fb, perFrameData);
    fb->setCompletedEvent(OSP_WORLD_RENDERED);

    fb->endFrame(renderer->errorThreshold, camera);
    fb->setCompletedEvent(OSP_FRAME_FINISHED);
  }
  return event;
}

std::string LocalTiledLoadBalancer::toString() const
{
  return "ospray::LocalTiledLoadBalancer";
}

} // namespace ospray
