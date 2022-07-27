// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../fb/DistributedFrameBuffer.h"
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
  DistributedLoadBalancer();
  ~DistributedLoadBalancer() override;
  void renderFrame(FrameBuffer *fb,
      Renderer *renderer,
      Camera *camera,
      World *world) override;

  void renderFrameReplicated(DistributedFrameBuffer *dfb,
      Renderer *renderer,
      Camera *camera,
      DistributedWorld *world);

  void setObjectHandle(ObjectHandle &handle_);

  /* Not implemented by Distributed load balancer currently,
   * this could potentially be useful to implement later to manage
   * the actual tile list rendering after computing the list of tiles
   * to be rendered by this rank in renderFrame
   */
  void runRenderTasks(FrameBuffer *fb,
      Renderer *renderer,
      Camera *camera,
      World *world,
      const utility::ArrayView<uint32_t> &renderTaskIDs,
      void *perFrameData) override;

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
