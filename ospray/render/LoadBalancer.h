// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "camera/Camera.h"
#include "common/OSPCommon.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
#include "render/Renderer.h"
#include "rkcommon/utility/ArrayView.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE TiledLoadBalancer
{
  virtual ~TiledLoadBalancer() = default;

  virtual std::string toString() const = 0;

  /*! Render the entire framebuffer using the given renderer, camera and
   * world configuration using the load balancer to parallelize the work
   */
  virtual void renderFrame(
      FrameBuffer *fb, Renderer *renderer, Camera *camera, World *world) = 0;

  /*! Render the specified subset of tiles using the given renderer, camera and
   * world configuration using the load balancer to parallelize the work
   */
  virtual void renderTiles(FrameBuffer *fb,
      Renderer *renderer,
      Camera *camera,
      World *world,
      const utility::ArrayView<int> &tileIDs,
      void *perFrameData) = 0;

  static size_t numJobs(const int spp, int accumID);
};

// Inlined definitions //////////////////////////////////////////////////////

inline size_t TiledLoadBalancer::numJobs(const int spp, int accumID)
{
  const int blocks = (accumID > 0 || spp > 0)
      ? 1
      : std::min(1 << -2 * spp, TILE_SIZE * TILE_SIZE);
  return divRoundUp(
      (TILE_SIZE * TILE_SIZE) / RENDERTILE_PIXELS_PER_JOB, blocks);
}

//! tiled load balancer for local rendering on the given machine
/*! a tiled load balancer that orchestrates (multi-threaded)
  rendering on a local machine, without any cross-node
  communication/load balancing at all (even if there are multiple
  application ranks each doing local rendering on their own)  */
struct OSPRAY_SDK_INTERFACE LocalTiledLoadBalancer : public TiledLoadBalancer
{
  void renderFrame(FrameBuffer *fb,
      Renderer *renderer,
      Camera *camera,
      World *world) override;

  void renderTiles(FrameBuffer *fb,
      Renderer *renderer,
      Camera *camera,
      World *world,
      const utility::ArrayView<int> &tileIDs,
      void *perFrameData) override;

  std::string toString() const override;
};

} // namespace ospray
