// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/OSPCommon.h"
#include "render/Renderer.h"

namespace ospray {

struct FrameBuffer;
struct Camera;
struct World;

struct OSPRAY_SDK_INTERFACE TiledLoadBalancer
{
  virtual ~TiledLoadBalancer() = default;

  virtual std::string toString() const = 0;

  /*! Render the entire framebuffer using the given renderer, camera and
   * world configuration using the load balancer to parallelize the work
   */
  virtual std::pair<AsyncEvent, AsyncEvent> renderFrame(FrameBuffer *fb,
      Renderer *renderer,
      Camera *camera,
      World *world,
      bool wait = true) = 0;
};

// Inlined definitions //////////////////////////////////////////////////////

//! tiled load balancer for local rendering on the given machine
/*! a tiled load balancer that orchestrates (multi-threaded)
  rendering on a local machine, without any cross-node
  communication/load balancing at all (even if there are multiple
  application ranks each doing local rendering on their own)  */
struct OSPRAY_SDK_INTERFACE LocalTiledLoadBalancer : public TiledLoadBalancer
{
  std::pair<AsyncEvent, AsyncEvent> renderFrame(FrameBuffer *fb,
      Renderer *renderer,
      Camera *camera,
      World *world,
      bool wait = true) override;

  std::string toString() const override;
};

} // namespace ospray
