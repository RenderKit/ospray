#include "loadbalancer.h"
#include "renderer.h"

namespace ospray {
  TiledLoadBalancer *TiledLoadBalancer::instance = NULL;

  void LocalTiledLoadBalancer::renderFrame(TileRenderer *tiledRenderer,
                                           FrameBuffer *fb)
  {
    Assert(tiledRenderer);
    Tile tile;
    tile.fbSize = fb->size;
    for (int y0=0;y0<fb->size.y;y0+=TILE_SIZE) {
      tile.region.lower.y = y0;
      tile.region.upper.y = std::min(y0+TILE_SIZE,fb->size.y);
      for (int x0=0;x0<fb->size.x;x0+=TILE_SIZE) {
        tile.region.lower.x = x0;
        tile.region.upper.x = std::min(x0+TILE_SIZE,fb->size.x);
        // PING; 
        // PRINT(tile.region.lower);
        tiledRenderer->renderTile(tile);
        //fb->setTile(tile);
      }
    }
  }
}
