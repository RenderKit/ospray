// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// own
#include "LoadBalancer.h"
#include "Renderer.h"
#include "api/Device.h"
#include "ospcommon/tasking/parallel_for.h"

namespace ospray {

  std::unique_ptr<TiledLoadBalancer> TiledLoadBalancer::instance;

  /*! render a frame via the tiled load balancer */
  float LocalTiledLoadBalancer::renderFrame(FrameBuffer *fb,
                                            Renderer *renderer,
                                            Camera * camera,
                                            World * world)
  {
    void *perFrameData = renderer->beginFrame(fb, world);
    bool cancel        = false;
    std::atomic<int> pixelsDone{0};
    const auto fbSize     = fb->getNumPixels();
    const float rcpPixels = 1.0f / (fbSize.x * fbSize.y);

    tasking::parallel_for(fb->getTotalTiles(), [&](int taskIndex) {
      if (cancel)
        return;
      const size_t numTiles_x = fb->getNumTiles().x;
      const size_t tile_y     = taskIndex / numTiles_x;
      const size_t tile_x     = taskIndex - tile_y * numTiles_x;
      const vec2i tileID(tile_x, tile_y);
      const int32 accumID = fb->accumID(tileID);

      // increment also for finished tiles
      vec2i numPixels =
          ospcommon::min(vec2i(TILE_SIZE), fbSize - tileID * TILE_SIZE);
      pixelsDone += numPixels.product();

      if (fb->tileError(tileID) <= renderer->errorThreshold)
        return;

#if TILE_SIZE > MAX_TILE_SIZE
      auto tilePtr = make_unique<Tile>(tileID, fbSize, accumID);
      auto &tile   = *tilePtr;
#else
      Tile __aligned(64) tile(tileID, fbSize, accumID);
#endif

      tasking::parallel_for(numJobs(renderer->spp, accumID), [&](size_t tIdx) {
        renderer->renderTile(fb, camera, world, perFrameData, tile, tIdx);
      });

      fb->setTile(tile);
      fb->reportProgress(pixelsDone * rcpPixels);

      if (fb->frameCancelled())
        cancel = true;
    });

    fb->setCompletedEvent(OSP_WORLD_RENDERED);

    renderer->endFrame(fb, perFrameData);

    fb->endFrame(renderer->errorThreshold, camera);

    return fb->getVariance();
  }

  std::string LocalTiledLoadBalancer::toString() const
  {
    return "ospray::LocalTiledLoadBalancer";
  }

}  // namespace ospray
