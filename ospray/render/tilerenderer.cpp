#include "tilerenderer.h"
#include "loadbalancer.h"

namespace ospray {

  void TileRenderer::renderFrame(FrameBuffer *fb)
  {
    TiledLoadBalancer::instance->renderFrame(this,fb);
  }
  
}
