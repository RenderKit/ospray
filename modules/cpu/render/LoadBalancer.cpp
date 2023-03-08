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

  renderer->renderTasks(fb,
      camera,
      world,
      perFrameData,
      fb->getRenderTaskIDs(renderer->errorThreshold)
#ifdef OSPRAY_TARGET_SYCL
          ,
      *syclQueue
#endif
  );

  renderer->endFrame(fb, perFrameData);

  fb->setCompletedEvent(OSP_WORLD_RENDERED);
  fb->endFrame(renderer->errorThreshold, camera);
  fb->setCompletedEvent(OSP_FRAME_FINISHED);
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
