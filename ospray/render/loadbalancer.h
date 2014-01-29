#pragma once

#include "../common/ospcommon.h"
#include "../fb/framebuffer.h"

namespace ospray {

  struct TileRenderer;

  struct TiledLoadBalancer 
  {
    static TiledLoadBalancer *instance;
    virtual void renderFrame(TileRenderer *tiledRenderer,
                             FrameBuffer *fb) = 0;
  };

  struct LocalTiledLoadBalancer 
  {
    virtual void renderFrame(TileRenderer *tiledRenderer,
                             FrameBuffer *fb);
  };
}
