// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
#include "ospray/common/tasking/parallel_for.h"
// ospc
#include "common/sysinfo.h"
// stl
#include <algorithm>

namespace ospray {

  using std::cout;
  using std::endl;

  TiledLoadBalancer *TiledLoadBalancer::instance = NULL;

  LocalTiledLoadBalancer::LocalTiledLoadBalancer()
#ifdef OSPRAY_TASKING_TBB
    : tbb_init(numThreads)
#endif
  {
  }

  /*! render a frame via the tiled load balancer */
  float LocalTiledLoadBalancer::renderFrame(Renderer *renderer,
                                            FrameBuffer *fb,
                                            const uint32 channelFlags)
  {
    Assert(renderer);
    Assert(fb);

    void *perFrameData = renderer->beginFrame(fb);

    int numTiles_x = divRoundUp(fb->size.x,TILE_SIZE);
    int numTiles_y = divRoundUp(fb->size.y,TILE_SIZE);

    const int NTASKS = numTiles_x * numTiles_y;

    parallel_for(NTASKS, [&](int taskIndex) {
      const size_t tile_y = taskIndex / numTiles_x;
      const size_t tile_x = taskIndex - tile_y*numTiles_x;
      const vec2i tileID(tile_x, tile_y);
      const int32 accumID = fb->accumID(tileID);

      if (fb->tileError(tileID) <= renderer->errorThreshold)
        return;

      Tile __aligned(64) tile(tileID, fb->size, accumID);

      parallel_for(numJobs(renderer->spp, accumID), [&](int tIdx) {
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

  /*! render a frame via the tiled load balancer */
  std::string InterleavedTiledLoadBalancer::toString() const
  {
    return "ospray::InterleavedTiledLoadBalancer";
  }

  float InterleavedTiledLoadBalancer::renderFrame(Renderer *renderer,
                                                  FrameBuffer *fb,
                                                  const uint32 channelFlags)
  {
    Assert(renderer);
    Assert(fb);

    void *perFrameData = renderer->beginFrame(fb);

    int numTiles_x     = divRoundUp(fb->size.x,TILE_SIZE);
    int numTiles_y     = divRoundUp(fb->size.y,TILE_SIZE);
    int numTiles_total = numTiles_x * numTiles_y;

    const int NTASKS = (numTiles_total / numDevices)
                       + (numTiles_total % numDevices > deviceID);

    parallel_for(NTASKS, [&](int taskIndex) {
      int tileIndex = deviceID + numDevices * taskIndex;
      const size_t tile_y = tileIndex / numTiles_x;
      const size_t tile_x = tileIndex - tile_y*numTiles_x;
      const vec2i tileID(tile_x, tile_y);
      const int32 accumID = fb->accumID(tileID);

      if (fb->tileError(tileID) <= renderer->errorThreshold)
        return;

#ifdef __MIC__
#  define MAX_TILE_SIZE 32
#else
#  define MAX_TILE_SIZE 128
#endif

#if TILE_SIZE>MAX_TILE_SIZE
      Tile *tilePtr = new Tile(tileID, fb->size, accumID);
      Tile &tile = *tilePtr;
#else
      Tile __aligned(64) tile(tileID, fb->size, accumID);
#endif

      parallel_for(numJobs(renderer->spp, accumID), [&](int tIdx) {
        renderer->renderTile(perFrameData, tile, tIdx);
      });

      fb->setTile(tile);
#if TILE_SIZE>MAX_TILE_SIZE
      delete tilePtr;
#endif
    });

    renderer->endFrame(perFrameData,channelFlags);

    return fb->endFrame(renderer->errorThreshold);
  }

} // ::ospray
