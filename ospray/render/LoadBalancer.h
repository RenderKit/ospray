// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/OSPCommon.h"
#include "fb/FrameBuffer.h"
#include "render/Renderer.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE TiledLoadBalancer
{
  static std::unique_ptr<TiledLoadBalancer> instance;

  virtual ~TiledLoadBalancer() = default;

  virtual std::string toString() const = 0;

  virtual float renderFrame(
      FrameBuffer *fb, Renderer *renderer, Camera *camera, World *world) = 0;

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
  float renderFrame(FrameBuffer *fb,
      Renderer *renderer,
      Camera *camera,
      World *world) override;

  std::string toString() const override;
};

} // namespace ospray
