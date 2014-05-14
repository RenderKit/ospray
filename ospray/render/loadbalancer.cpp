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
    fb = NULL;
    refDec();
  }

  void LocalTiledLoadBalancer::RenderTask::run(size_t threadIndex, 
                                               size_t threadCount, 
                                               size_t taskIndex, 
                                               size_t taskCount, 
                                               TaskScheduler::Event* event) 
  {
    Tile tile;
    tile.fbSize = fb->size;
    tile.rcp_fbSize = rcp(vec2f(fb->size));
    const size_t tile_y = taskIndex / numTiles_x;
    const size_t tile_x = taskIndex - tile_y*numTiles_x; //taskIndex % numTiles_x;
    tile.region.lower.x = tile_x * TILE_SIZE; //x0;
    tile.region.lower.y = tile_y * TILE_SIZE; //y0;
    tile.region.upper.x = std::min(tile.region.lower.x+TILE_SIZE,fbSize.x);
    tile.region.upper.y = std::min(tile.region.lower.y+TILE_SIZE,fbSize.y);
    frameRenderJob->renderTile(tile);
    Assert(TiledLoadBalancer::instance);
    fb->setTile(tile);
  }

  /*! render a frame via the tiled load balancer */
  void LocalTiledLoadBalancer::renderFrame(TileRenderer *tiledRenderer,
                                           FrameBuffer *fb)
  {
    Assert(tiledRenderer);
    Assert(fb);

    TileRenderer::RenderJob *job = tiledRenderer->createRenderJob(fb);
    RenderTask *renderTask  = new RenderTask(fb,job);
    if (fb->renderTask) {
      fb->renderTask->done.sync();
      fb->renderTask = NULL;
    }
    fb->renderTask = renderTask;
    TaskScheduler::addTask(-1, TaskScheduler::GLOBAL_BACK, &renderTask->task); 
#if 1
    // do synchronous rendering: directly wait after each frame
    fb->renderTask->done.sync();
#endif
  }






  InterleavedTiledLoadBalancer::RenderTask::RenderTask(FrameBuffer *fb,
                                                       TileRenderer::RenderJob *frameRenderJob,
                                                       size_t numTiles_x,
                                                       size_t numTiles_y,
                                                       size_t numTiles_mine,
                                                       size_t deviceID,
                                                       size_t numDevices)
    : fb(fb),
      numTiles_mine(numTiles_mine),
      deviceID(deviceID),
      numDevices(numDevices),
      numTiles_x(numTiles_x),
      numTiles_y(numTiles_y),
      fbSize(fb->size),
      task(&done,_run,this,
           numTiles_mine,
           _finish,this,"InterleavedTiledLoadBalancer::RenderTask"),
      frameRenderJob(frameRenderJob)
  {
    refInc();
  }

  void InterleavedTiledLoadBalancer::RenderTask::finish(size_t threadIndex, 
                                                        size_t threadCount, 
                                                        TaskScheduler::Event* event) 
  {
    frameRenderJob = NULL;
    fb = NULL;
    refDec();
  }
  
  void InterleavedTiledLoadBalancer::RenderTask::run(size_t threadIndex, 
                                                     size_t threadCount, 
                                                     size_t taskIndex, 
                                                     size_t taskCount, 
                                                     TaskScheduler::Event* event) 
  {

    Tile tile;
    tile.fbSize = fb->size;
    tile.rcp_fbSize = rcp(vec2f(fb->size));

    const size_t tileIndex = taskIndex * numDevices + deviceID;
    const size_t tile_y = tileIndex / numTiles_x;
    const size_t tile_x = tileIndex - tile_y*numTiles_x; //taskIndex % numTiles_x;
    tile.region.lower.x = tile_x * TILE_SIZE; //x0;
    tile.region.lower.y = tile_y * TILE_SIZE; //y0;

    tile.region.upper.x = std::min(tile.region.lower.x+TILE_SIZE,fbSize.x);
    tile.region.upper.y = std::min(tile.region.lower.y+TILE_SIZE,fbSize.y);

    frameRenderJob->renderTile(tile);
    Assert(TiledLoadBalancer::instance);
    fb->setTile(tile);
  }

  /*! render a frame via the tiled load balancer */
  void InterleavedTiledLoadBalancer::renderFrame(TileRenderer *tiledRenderer,
                                           FrameBuffer *fb)
  {
    Assert(tiledRenderer);
    Assert(fb);

    TileRenderer::RenderJob *job = tiledRenderer->createRenderJob(fb);
    size_t numTiles_x = divRoundUp(fb->size.x,TILE_SIZE);
    size_t numTiles_y = divRoundUp(fb->size.y,TILE_SIZE);
    size_t numTiles_total = numTiles_x*numTiles_y;
    size_t numTiles_mine
      = (numTiles_total / numDevices)
      + (numTiles_total % numDevices > deviceID);
    RenderTask *renderTask  = new RenderTask(fb,job,numTiles_x,numTiles_y,
                                             numTiles_mine,deviceID,numDevices);
    
    // wait for old render task to finish
    if (fb->renderTask) {
      fb->renderTask->done.sync();
      fb->renderTask = NULL;
    }

    // and start new render task
    fb->renderTask = renderTask;
    TaskScheduler::addTask(-1, TaskScheduler::GLOBAL_BACK, &renderTask->task); 
#if 1
    // do synchronous rendering: directly wait after each frame
    fb->renderTask->done.sync();
#endif
  }
}
