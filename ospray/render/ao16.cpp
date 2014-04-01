// ospray
#include "ao16.h"
#include "../camera/perspectivecamera.h"
// embree
#include "common/sys/sync/atomic.h"

namespace ospray {
  extern "C" void ispc_AO16Renderer_renderTile(void *tile, void *camera, RTCScene scene);

  void AO16Renderer::RenderTask::renderTile(Tile &tile)
  {
    ispc_AO16Renderer_renderTile(&tile,camera->getIE(),world->eScene);
  }

  TileRenderer::RenderJob *AO16Renderer::createRenderJob(FrameBuffer *fb)
  {
    RenderTask *frame = new RenderTask;
    frame->world  = (Model  *)getParam("world",NULL);
    frame->camera = (Camera *)getParam("camera",NULL);
    return frame;
  }

  OSP_REGISTER_RENDERER(AO16Renderer,ao16);
};

