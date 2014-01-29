#include "loadbalancer.h"
#include "renderer.h"

namespace ospray {
  TiledLoadBalancer *TiledLoadBalancer::instance = NULL;

#define divRoundUp(X,Y) ((X)+(Y)-1)/(Y)
  
  LocalTiledLoadBalancer::RenderTileTask::RenderTileTask(size_t numTiles, 
                                                         LocalTiledLoadBalancer *loadBalancer,
                                                         TileRenderer *tiledRenderer,
                                                         FrameBuffer *fb)
    : Task(&eventSync,_run,this,numTiles,NULL,NULL,
           "LocalTiledLoadBalancer::RenderTileTask"),
      loadBalancer(loadBalancer), fb(fb), tiledRenderer(tiledRenderer)
  {
  }

  void LocalTiledLoadBalancer::RenderTileTask::run(size_t threadIndex, 
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
    fb->setTile(tile);
  }

  /*! render a frame via the tiled load balancer */
  void LocalTiledLoadBalancer::renderFrame(TileRenderer *tiledRenderer,
                                           FrameBuffer *fb)
  {
    Assert(tiledRenderer);
    size_t numTiles = divRoundUp(fb->size.x,TILE_SIZE)*divRoundUp(fb->size.y,TILE_SIZE);
    RenderTileTask *renderTask = new RenderTileTask(numTiles,this,tiledRenderer,fb);
    TaskScheduler::addTask(-1, TaskScheduler::GLOBAL_BACK, renderTask); //&task_renderTiles)
    renderTask->eventSync.sync();
  }
}
