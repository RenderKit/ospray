// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "ospcommon/tasking/parallel_for.h"

namespace ospray {

  std::unique_ptr<TiledLoadBalancer> TiledLoadBalancer::instance {};

  /*! render a frame via the tiled load balancer */
  float LocalTiledLoadBalancer::renderFrame(Renderer *renderer,
                                            FrameBuffer *fb,
                                            const uint32 channelFlags)
  {
    Assert(renderer);
    Assert(fb);

    void *perFrameData = renderer->beginFrame(fb);

    tasking::parallel_for(fb->getTotalTiles(), [&](int taskIndex) {
      const size_t numTiles_x = fb->getNumTiles().x;
      const size_t tile_y = taskIndex / numTiles_x;
      const size_t tile_x = taskIndex - tile_y*numTiles_x;
      const vec2i tileID(tile_x, tile_y);
      const int32 accumID = fb->accumID(tileID);

      if (fb->tileError(tileID) <= renderer->errorThreshold)
        return;

#define MAX_TILE_SIZE 128
#if TILE_SIZE > MAX_TILE_SIZE
      auto tilePtr = make_unique<Tile>(tileID, fb->size, accumID);
      auto &tile   = *tilePtr;
#else
      Tile __aligned(64) tile(tileID, fb->size, accumID);
#endif

      tasking::parallel_for(numJobs(renderer->spp, accumID), [&](int tIdx) {
        renderer->renderTile(perFrameData, tile, tIdx);
      });

      fb->setTile(tile);
    });

    renderer->endFrame(perFrameData,channelFlags);

    return fb->endFrame(renderer->errorThreshold);
  }

  std::string LocalTiledLoadBalancer::toString() const
  {
    return "ospray::LocalTiledLoadBalancer";
  }

} // ::ospray
