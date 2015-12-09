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

namespace ospray {

  using std::cout;
  using std::endl;

  TiledLoadBalancer *TiledLoadBalancer::instance = NULL;

  void LocalTiledLoadBalancer::RenderTask::finish()
  {
    renderer->endFrame(perFrameData,channelFlags);
    renderer = NULL;
    fb = NULL;
  }

  void LocalTiledLoadBalancer::RenderTask::run(size_t taskIndex) 
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

    renderer->renderTile(perFrameData,tile);
    fb->setTile(tile);
  }

  /*! render a frame via the tiled load balancer */
  void LocalTiledLoadBalancer::renderFrame(Renderer *tiledRenderer,
                                           FrameBuffer *fb,
                                           const uint32 channelFlags)
  {
    Assert(tiledRenderer);
    Assert(fb);

    void *perFrameData = tiledRenderer->beginFrame(fb);

    Ref<RenderTask> renderTask = new RenderTask;
    renderTask->fb = fb;
    renderTask->perFrameData = perFrameData; 
    renderTask->renderer = tiledRenderer;
    renderTask->numTiles_x = divRoundUp(fb->size.x,TILE_SIZE);
    renderTask->numTiles_y = divRoundUp(fb->size.y,TILE_SIZE);
    renderTask->channelFlags = channelFlags;
    
    renderTask->schedule(renderTask->numTiles_x*renderTask->numTiles_y);
    renderTask->wait();

    // /*! iw: using a local sync event for now; "in theory" we should be
    //     able to attach something like a sync event to the frame
    //     buffer, just trigger the task here, and let somebody else sync
    //     on the framebuffer once it is needed; alas, I'm currently
    //     running into some issues with the embree taks system when
    //     trying to do so, and thus am reverting to this
    //     fully-synchronous version for now */

    // // renderTask->fb->frameIsReadyEvent = TaskScheduler::EventSync();
    // TaskScheduler::EventSync sync;
    // renderTask->task = embree::TaskScheduler::Task
    //   (&sync,
    //   // (&renderTask->fb->frameIsReadyEvent,
    //    renderTask->_run,renderTask.ptr,
    //    renderTask->numTiles_x*renderTask->numTiles_y,
    //    renderTask->_finish,renderTask.ptr,
    //    "LocalTiledLoadBalancer::RenderTask");
    // TaskScheduler::addTask(-1, TaskScheduler::GLOBAL_BACK, &renderTask->task); 
    // sync.sync();
  }

  void InterleavedTiledLoadBalancer::RenderTask::run(size_t taskIndex)
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

    renderer->renderTile(perFrameData,tile);
  }


  void InterleavedTiledLoadBalancer::RenderTask::finish()
  {
    renderer->endFrame(perFrameData,channelFlags);
    renderer = NULL;
    fb = NULL;
    // refDec();
  }

  /*! render a frame via the tiled load balancer */
  void InterleavedTiledLoadBalancer::renderFrame(Renderer *tiledRenderer,
                                                 FrameBuffer *fb,
                                                 const uint32 channelFlags)
  {
    Assert(tiledRenderer);
    Assert(fb);

    void *perFrameData = tiledRenderer->beginFrame(fb);

    Ref<RenderTask> renderTask = new RenderTask;
    renderTask->fb = fb;
    renderTask->perFrameData = perFrameData;
    renderTask->renderer = tiledRenderer;
    renderTask->numTiles_x = divRoundUp(fb->size.x,TILE_SIZE);
    renderTask->numTiles_y = divRoundUp(fb->size.y,TILE_SIZE);
    size_t numTiles_total = renderTask->numTiles_x*renderTask->numTiles_y;
    
    renderTask->numTiles_mine
      = (numTiles_total / numDevices)
      + (numTiles_total % numDevices > deviceID);
    renderTask->channelFlags = channelFlags;
    renderTask->deviceID     = deviceID;
    renderTask->numDevices   = numDevices;
    
    renderTask->schedule(renderTask->numTiles_mine);
    renderTask->wait();
  }

} // ::ospray
