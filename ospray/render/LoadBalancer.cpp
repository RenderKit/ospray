// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

  void LocalTiledLoadBalancer::RenderTask::finish() const
  {
    renderer->endFrame(perFrameData,channelFlags);
    renderer = NULL;
    fb = NULL;
  }

  void LocalTiledLoadBalancer::RenderTask::run(size_t taskIndex) const
  {
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
  }

  LocalTiledLoadBalancer::LocalTiledLoadBalancer()
#ifdef OSPRAY_USE_TBB
    : tbb_init(numThreads)
#endif
  {
  }

  /*! render a frame via the tiled load balancer */
  void LocalTiledLoadBalancer::renderFrame(Renderer *tiledRenderer,
                                           FrameBuffer *fb,
                                           const uint32 channelFlags)
  {
    Assert(tiledRenderer);
    Assert(fb);

    void *perFrameData = tiledRenderer->beginFrame(fb);

    RenderTask renderTask;
    renderTask.fb = fb;
    renderTask.renderer = tiledRenderer;
    renderTask.perFrameData = perFrameData;
    renderTask.numTiles_x = divRoundUp(fb->size.x,TILE_SIZE);
    renderTask.numTiles_y = divRoundUp(fb->size.y,TILE_SIZE);
    renderTask.channelFlags = channelFlags;

    const int NTASKS = renderTask.numTiles_x * renderTask.numTiles_y;
    parallel_for(NTASKS, [&](int taskIndex){renderTask.run(taskIndex);});

    renderTask.finish();
  }

  std::string LocalTiledLoadBalancer::toString() const
  {
    return "ospray::LocalTiledLoadBalancer";
  }

  void InterleavedTiledLoadBalancer::RenderTask::run(size_t taskIndex) const
  {
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
  }


  void InterleavedTiledLoadBalancer::RenderTask::finish() const
  {
    renderer->endFrame(perFrameData,channelFlags);
    renderer = NULL;
    fb = NULL;
  }

  /*! render a frame via the tiled load balancer */
  void InterleavedTiledLoadBalancer::renderFrame(Renderer *tiledRenderer,
                                                 FrameBuffer *fb,
                                                 const uint32 channelFlags)
  {
    Assert(tiledRenderer);
    Assert(fb);

    void *perFrameData = tiledRenderer->beginFrame(fb);

    RenderTask renderTask;
    renderTask.fb = fb;
    renderTask.perFrameData = perFrameData;
    renderTask.renderer     = tiledRenderer;
    renderTask.numTiles_x   = divRoundUp(fb->size.x,TILE_SIZE);
    renderTask.numTiles_y   = divRoundUp(fb->size.y,TILE_SIZE);
    size_t numTiles_total   = renderTask.numTiles_x * renderTask.numTiles_y;

    renderTask.numTiles_mine
      = (numTiles_total / numDevices)
      + (numTiles_total % numDevices > deviceID);
    renderTask.channelFlags = channelFlags;
    renderTask.deviceID     = deviceID;
    renderTask.numDevices   = numDevices;

    const int NTASKS = renderTask.numTiles_mine;
    parallel_for(NTASKS, [&](int taskIndex){renderTask.run(taskIndex);});

    // NOTE(jda) - this line was added to match LocalTiledLoadBalancer...check!
    //renderTask.finish();
  }

} // ::ospray
