#include "loadbalancer.h"
#include "renderer.h"

namespace ospray {
  TiledLoadBalancer *TiledLoadBalancer::instance = NULL;

  LocalTiledLoadBalancer::RenderTask::RenderTask(size_t numTiles, 
                                                 TileRenderer *tiledRenderer,
                                                 FrameBuffer *fb)
    : task(&done,_run,this,numTiles,_finish,this,"LocalTiledLoadBalancer::RenderTask"),
      fb(fb), 
      tiledRenderer(tiledRenderer)
  {
  }

  void LocalTiledLoadBalancer::RenderTask::finish(size_t threadIndex, 
                                                  size_t threadCount, 
                                                  TaskScheduler::Event* event) 
  {
    // release the refcounts:
    fb = NULL; 
    tiledRenderer = NULL;
  }

  void LocalTiledLoadBalancer::RenderTask::run(size_t threadIndex, 
                                               size_t threadCount, 
                                               size_t taskIndex, 
                                               size_t taskCount, 
                                               TaskScheduler::Event* event) 
  {
    Tile tile;
    tile.fbSize = fb->size;
    const size_t numTiles_x = divRoundUp(fb->size.x,TILE_SIZE);
    const size_t tile_y = taskIndex / numTiles_x;
    const size_t tile_x = taskIndex % numTiles_x;
    tile.region.lower.x = tile_x * TILE_SIZE; //x0;
    tile.region.lower.y = tile_y * TILE_SIZE; //y0;
    tile.region.upper.x = std::min(tile.region.lower.x+TILE_SIZE,fb->size.x);
    tile.region.upper.y = std::min(tile.region.lower.y+TILE_SIZE,fb->size.y);
    tiledRenderer->renderTile(tile);
    Assert(TiledLoadBalancer::instance);
    TiledLoadBalancer::instance->returnTile(fb.ptr,tile);
    fb->setTile(tile);
  }

  /*! render a frame via the tiled load balancer */
  void LocalTiledLoadBalancer::renderFrame(TileRenderer *tiledRenderer,
                                           FrameBuffer *fb)
  {
    Assert(tiledRenderer);
    Assert(fb);

    if (fb->renderTask) {
      PING;
      fb->renderTask->done.sync();
      fb->renderTask = NULL;
    }
    
    size_t numTiles = divRoundUp(fb->size.x,TILE_SIZE)*divRoundUp(fb->size.y,TILE_SIZE);
    RenderTask *renderTask = new RenderTask(numTiles,tiledRenderer,fb);
    fb->renderTask = renderTask;
    TaskScheduler::addTask(-1, TaskScheduler::GLOBAL_BACK, &renderTask->task); 
    printf("scheduling %lx / %lx\n",fb,fb->renderTask.ptr);
  }

  /*! the _local_ tiled load balancer has nothing to do when a tile is
      done; all synchronization is handled by the render task, and all
      generated tiles automtically wirtten into the local frame
      buffer */
  void LocalTiledLoadBalancer::returnTile(FrameBuffer *fb, Tile &tile)
  {
  }
}
