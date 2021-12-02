// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LoadBalancer.h"
#include "Renderer.h"
#include "api/Device.h"
#include "rkcommon/tasking/parallel_for.h"

namespace ospray {

void LocalTiledLoadBalancer::renderFrame(
    FrameBuffer *fb, Renderer *renderer, Camera *camera, World *world)
{
  fb->beginFrame();
  void *perFrameData = renderer->beginFrame(fb, world);

  renderTiles(fb, renderer, camera, world, fb->getTileIDs(), perFrameData);

  renderer->endFrame(fb, perFrameData);

  fb->setCompletedEvent(OSP_WORLD_RENDERED);
  fb->endFrame(renderer->errorThreshold, camera);
  fb->setCompletedEvent(OSP_FRAME_FINISHED);
}

void LocalTiledLoadBalancer::renderTiles(FrameBuffer *fb,
    Renderer *renderer,
    Camera *camera,
    World *world,
    const utility::ArrayView<int> &tileIDs,
    void *perFrameData)
{
  bool cancel = false;
  std::atomic<int> pixelsDone{0};

  const auto fbSize = fb->getNumPixels();
  const float rcpPixels = 1.0f / (TILE_SIZE * TILE_SIZE * tileIDs.size());

#ifdef OSPRAY_SERIAL_RENDERING
  tasking::serial_for(tileIDs.size(), [&](size_t taskIndex) {
#else
  tasking::parallel_for(tileIDs.size(), [&](size_t taskIndex) {
#endif
    if (cancel)
      return;
    const size_t numTiles_x = fb->getNumTiles().x;
    const size_t tile_y = tileIDs[taskIndex] / numTiles_x;
    const size_t tile_x = tileIDs[taskIndex] - tile_y * numTiles_x;
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
    // TODO: Maybe change progress reporting to sum an atomic float instead?
    // with multidevice there will be many parallel updates to report progress,
    // each reporting their progress on some subset of pixels.
    fb->reportProgress(pixelsDone * rcpPixels);

    if (fb->frameCancelled())
      cancel = true;
  });
}

std::string LocalTiledLoadBalancer::toString() const
{
  return "ospray::LocalTiledLoadBalancer";
}

} // namespace ospray
