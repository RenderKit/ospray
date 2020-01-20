// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// own
#include "LoadBalancer.h"
#include "Renderer.h"
#include "api/Device.h"
#include "ospcommon/tasking/parallel_for.h"

namespace ospray {

std::unique_ptr<TiledLoadBalancer> TiledLoadBalancer::instance;

/*! render a frame via the tiled load balancer */
float LocalTiledLoadBalancer::renderFrame(
    FrameBuffer *fb, Renderer *renderer, Camera *camera, World *world)
{
  bool cancel = false;
  std::atomic<int> pixelsDone{0};

  const auto fbSize = fb->getNumPixels();
  const float rcpPixels = 1.0f / (fbSize.x * fbSize.y);

  fb->beginFrame();
  void *perFrameData = renderer->beginFrame(fb, world);

#ifdef OSPRAY_SERIAL_RENDERING
  tasking::serial_for(fb->getTotalTiles(), [&](int taskIndex) {
#else
  tasking::parallel_for(fb->getTotalTiles(), [&](int taskIndex) {
#endif
    if (cancel)
      return;
    const size_t numTiles_x = fb->getNumTiles().x;
    const size_t tile_y = taskIndex / numTiles_x;
    const size_t tile_x = taskIndex - tile_y * numTiles_x;
    const vec2i tileID(tile_x, tile_y);
    const int32 accumID = fb->accumID(tileID);

    // increment also for finished tiles
    vec2i numPixels = min(vec2i(TILE_SIZE), fbSize - tileID * TILE_SIZE);
    pixelsDone += numPixels.product();

    if (fb->tileError(tileID) <= renderer->errorThreshold)
      return;

#if TILE_SIZE > MAX_TILE_SIZE
    auto tilePtr = make_unique<Tile>(tileID, fbSize, accumID);
    auto &tile = *tilePtr;
#else
    Tile __aligned(64) tile(tileID, fbSize, accumID);
#endif

#ifdef OSPRAY_SERIAL_RENDERING
    tasking::serial_for(numJobs(renderer->spp, accumID), [&](size_t tIdx) {
#else
    tasking::parallel_for(numJobs(renderer->spp, accumID), [&](size_t tIdx) {
#endif
      renderer->renderTile(fb, camera, world, perFrameData, tile, tIdx);
    });

    fb->setTile(tile);
    fb->reportProgress(pixelsDone * rcpPixels);

    if (fb->frameCancelled())
      cancel = true;
  });

  renderer->endFrame(fb, perFrameData);

  fb->setCompletedEvent(OSP_WORLD_RENDERED);
  fb->endFrame(renderer->errorThreshold, camera);
  fb->setCompletedEvent(OSP_FRAME_FINISHED);

  return fb->getVariance();
}

std::string LocalTiledLoadBalancer::toString() const
{
  return "ospray::LocalTiledLoadBalancer";
}

} // namespace ospray
