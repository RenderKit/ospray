// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/DistributedWorld.h"
#include "../fb/DistributedFrameBuffer.h"
#include "camera/Camera.h"
#include "common/World.h"
#include "render/LoadBalancer.h"

namespace ospray {
namespace mpi {
namespace staticLoadBalancer {

/* The distributed load balancer manages both data and image
 * parallel distributed rendering, based on the renderer and
 * world configuration. Distributed renderers will use distributed
 * rendering, while a non-distributed renderer with a world with
 * one global region will give image-parallel rendering
 */
struct Distributed : public TiledLoadBalancer
{
  void renderFrame(FrameBuffer *fb,
      Renderer *renderer,
      Camera *camera,
      World *world) override;

  void renderFrameReplicated(DistributedFrameBuffer *dfb,
      Renderer *renderer,
      Camera *camera,
      DistributedWorld *world);

  std::string toString() const override;
};

} // namespace staticLoadBalancer
} // namespace mpi
} // namespace ospray
