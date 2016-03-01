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

#include "LoadBalancer.h"
#include "Renderer.h"
#include <sys/sysinfo.h>

// stl
#include <algorithm>

#include "ospray/common/parallel_for.h"

namespace ospray {

  using std::cout;
  using std::endl;

  TiledLoadBalancer *TiledLoadBalancer::instance = NULL;

  LocalTiledLoadBalancer::LocalTiledLoadBalancer()
#ifdef OSPRAY_USE_TBB
    : tbb_init(numThreads)
#endif
  {
  }

  /*! render a frame via the tiled load balancer */
  void LocalTiledLoadBalancer::renderFrame(Renderer *renderer,
                                           FrameBuffer *fb,
                                           const uint32 channelFlags)
  {
    Assert(renderer);
    Assert(fb);

    void *perFrameData = renderer->beginFrame(fb);

    int numTiles_x = divRoundUp(fb->size.x,TILE_SIZE);
    int numTiles_y = divRoundUp(fb->size.y,TILE_SIZE);

    const int NTASKS = numTiles_x * numTiles_y;

    parallel_for(NTASKS, [&](int taskIndex){
      Tile tile;
      const size_t tile_y = taskIndex / numTiles_x;
      const size_t tile_x = taskIndex - tile_y*numTiles_x;
      tile.region.lower.x = tile_x * TILE_SIZE;
      tile.region.lower.y = tile_y * TILE_SIZE;
      tile.region.upper.x = std::min(tile.region.lower.x+TILE_SIZE,fb->size.x);
      tile.region.upper.y = std::min(tile.region.lower.y+TILE_SIZE,fb->size.y);
      tile.fbSize = fb->size;
      tile.rcp_fbSize = rcp(vec2f(tile.fbSize));
      tile.generation = 0;
      tile.children = 0;

      const int spp = renderer->spp;
      const int blocks = (fb->accumID > 0 || spp > 0) ? 1 :
                         std::min(1 << -2 * spp, TILE_SIZE*TILE_SIZE);
      const size_t numJobs = ((TILE_SIZE*TILE_SIZE)/
                              RENDERTILE_PIXELS_PER_JOB + blocks-1)/blocks;

      parallel_for(numJobs, [&](int taskIndex){
        renderer->renderTile(perFrameData, tile, taskIndex);
      });

      fb->setTile(tile);
    });

    renderer->endFrame(perFrameData,channelFlags);
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

  void InterleavedTiledLoadBalancer::renderFrame(Renderer *renderer,
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

    parallel_for(NTASKS, [&](int taskIndex){
      int tileIndex = deviceID + numDevices * taskIndex;

      Tile tile;
      const size_t tile_y = tileIndex / numTiles_x;
      const size_t tile_x = tileIndex - tile_y*numTiles_x;
      tile.region.lower.x = tile_x * TILE_SIZE;
      tile.region.lower.y = tile_y * TILE_SIZE;
      tile.region.upper.x = std::min(tile.region.lower.x+TILE_SIZE,fb->size.x);
      tile.region.upper.y = std::min(tile.region.lower.y+TILE_SIZE,fb->size.y);
      tile.fbSize = fb->size;
      tile.rcp_fbSize = rcp(vec2f(tile.fbSize));
      tile.generation = 0;
      tile.children = 0;

      const int spp = renderer->spp;
      const int blocks = (fb->accumID > 0 || spp > 0) ? 1 :
                         std::min(1 << -2 * spp, TILE_SIZE*TILE_SIZE);
      const size_t numJobs = ((TILE_SIZE*TILE_SIZE)/
                              RENDERTILE_PIXELS_PER_JOB + blocks-1)/blocks;

      parallel_for(numJobs, [&](int taskIndex){
        renderer->renderTile(perFrameData, tile, taskIndex);
      });

      fb->setTile(tile);
    });

    renderer->endFrame(perFrameData,channelFlags);
  }

} // ::ospray
