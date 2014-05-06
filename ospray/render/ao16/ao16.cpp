// ospray
#include "ao16.h"
#include "ospray/camera/camera.h"
// embree
#include "common/sys/sync/atomic.h"

namespace ospray {
  extern "C" void ispc_AO16Renderer_renderTile(void *tile, void *camera, RTCScene scene);

  void AO16Renderer::RenderTask::renderTile(Tile &tile)
  {
    ispc_AO16Renderer_renderTile(&tile,camera->getIE(),world->embreeSceneHandle);
  }

  TileRenderer::RenderJob *AO16Renderer::createRenderJob(FrameBuffer *fb)
  {
    RenderTask *frame = new RenderTask;
    frame->world  = (Model  *)getParamObject("world",NULL); // old naming
    frame->world  = (Model  *)getParamObject("model",frame->world); // new naming
    frame->camera = (Camera *)getParamObject("camera",NULL);
    return frame;
  }

  OSP_REGISTER_RENDERER(AO16Renderer,ao16);
};

