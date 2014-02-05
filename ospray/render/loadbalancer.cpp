#include "loadbalancer.h"
#include "renderer.h"

namespace ospray {
  TiledLoadBalancer *TiledLoadBalancer::instance = NULL;

  LocalTiledLoadBalancer::RenderTask::RenderTask(FrameBuffer *fb,
                                                 TileRenderer::RenderJob *frameRenderJob)
    : fb(fb),
      numTiles_x(divRoundUp(fb->size.x,TILE_SIZE)),
      numTiles_y(divRoundUp(fb->size.y,TILE_SIZE)),
      fbSize(fb->size),
      task(&done,_run,this,
           divRoundUp(fb->size.x,TILE_SIZE)*divRoundUp(fb->size.y,TILE_SIZE),
           _finish,this,"LocalTiledLoadBalancer::RenderTask"),
      frameRenderJob(frameRenderJob)
  {
    refInc();
  }

  void LocalTiledLoadBalancer::RenderTask::finish(size_t threadIndex, 
                                                  size_t threadCount, 
                                                  TaskScheduler::Event* event) 
  {
    frameRenderJob = NULL;
    refDec();
  }

  void LocalTiledLoadBalancer::RenderTask::run(size_t threadIndex, 
                                               size_t threadCount, 
                                               size_t taskIndex, 
                                               size_t taskCount, 
                                               TaskScheduler::Event* event) 
  {
     // printf("tile %i %i\n",taskIndex,taskCount);
    Tile tile;
    tile.fbSize = fb->size;
    // const size_t numTiles_x = divRoundUp(fb->size.x,TILE_SIZE);
    // PRINT(numTiles_x);
    // PRINT(numTiles_y);
    // PRINT(taskIndex);
    const size_t tile_y = taskIndex / numTiles_x;
    const size_t tile_x = taskIndex - tile_y*numTiles_x; //taskIndex % numTiles_x;
    tile.region.lower.x = tile_x * TILE_SIZE; //x0;
    tile.region.lower.y = tile_y * TILE_SIZE; //y0;
    tile.region.upper.x = std::min(tile.region.lower.x+TILE_SIZE,fbSize.x);
    tile.region.upper.y = std::min(tile.region.lower.y+TILE_SIZE,fbSize.y);
    // PRINT(tile.region);
    frameRenderJob->renderTile(tile);
    Assert(TiledLoadBalancer::instance);
    // TiledLoadBalancer::instance->returnTile(fb.ptr,tile);
    // PRINT(tile.r[0]);
    fb->setTile(tile);
  }

  /*! render a frame via the tiled load balancer */
  void LocalTiledLoadBalancer::renderFrame(TileRenderer *tiledRenderer,
                                           FrameBuffer *fb)
  {
    Assert(tiledRenderer);
    Assert(fb);

    if (fb->renderTask) {
      fb->renderTask->done.sync();
      fb->renderTask = NULL;
    }
    // printf("rendering fb %lx\n",fb);
    RenderTask *renderTask  = new RenderTask(fb,tiledRenderer->createRenderJob(fb));
    fb->renderTask = renderTask;
    TaskScheduler::addTask(-1, TaskScheduler::GLOBAL_BACK, &renderTask->task); 
    fb->renderTask->done.sync();
    // printf("done rendering fb %lx\n",fb);
  }

  // /*! the _local_ tiled load balancer has nothing to do when a tile is
  //     done; all synchronization is handled by the render task, and all
  //     generated tiles automtically wirtten into the local frame
  //     buffer */
  // void LocalTiledLoadBalancer::returnTile(FrameBuffer *fb, Tile &tile)
  // {
  // }
}
