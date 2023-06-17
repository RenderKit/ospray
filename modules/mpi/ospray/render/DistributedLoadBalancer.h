// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef OSPRAY_TARGET_SYCL
#include <sycl/sycl.hpp>
#endif

#include "../fb/DistributedFrameBuffer.h"
#include "ObjectHandle.h"
#include "camera/Camera.h"
#include "common/World.h"
#include "render/LoadBalancer.h"

namespace ospray {
namespace mpi {

struct DistributedWorld;

/* The distributed load balancer manages both data and image
 * parallel distributed rendering, based on the renderer and
 * world configuration. Distributed renderers will use distributed
 * rendering, while a non-distributed renderer with a world with
 * one global region will give image-parallel rendering
 */
struct DistributedLoadBalancer : public TiledLoadBalancer
{
  DistributedLoadBalancer(ObjectHandle handle);

  ~DistributedLoadBalancer() override;

  std::pair<AsyncEvent, AsyncEvent> renderFrame(FrameBuffer *fb,
      Renderer *renderer,
      Camera *camera,
      World *world,
      bool wait = true) override;

  void renderFrameReplicated(DistributedFrameBuffer *dfb,
      Renderer *renderer,
      Camera *camera,
      DistributedWorld *world);

  std::string toString() const override;

 private:
  void renderFrameReplicatedDynamicLB(DistributedFrameBuffer *dfb,
      Renderer *renderer,
      Camera *camera,
      DistributedWorld *world,
      void *perFrameData);

  void renderFrameReplicatedStaticLB(DistributedFrameBuffer *dfb,
      Renderer *renderer,
      Camera *camera,
      DistributedWorld *world,
      void *perFrameData);

  ObjectHandle handle;
};

} // namespace mpi
} // namespace ospray
