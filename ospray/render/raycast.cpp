// ospray
#include "raycast.h"
#include "../camera/perspectivecamera.h"
// embree
#include "common/sys/sync/atomic.h"

namespace ospray {
  extern "C" void ispc__RayCastRenderer_renderFrame(void *camera, void *scene, void *fb);
  extern "C" void ispc__RayCastRenderer_renderTile(void *tile, void *camera, void *scene);

  void RayCastRenderer::RenderTask::renderTile(Tile &tile)
  {
    // PING;
    // PRINT(tile.region);
    // PRINT(tile.region);
    ispc__RayCastRenderer_renderTile(&tile,_camera,_scene);
    // PRINT(tile.r[0]);
    // PRINT(tile.g[0]);
    // PRINT(tile.b[0]);
  }

  TileRenderer::RenderJob *RayCastRenderer::createRenderJob(FrameBuffer *fb)
  {
    RenderTask *frame = new RenderTask;
    frame->world = (Model *)getParam("world",NULL);
    Assert2(frame->world,"null world handle (did you forget to assign a 'world' parameter to the ray_cast renderer?)");
    frame->_scene = (void*)frame->world->eScene;
    Assert2(frame->_scene,"invalid model without an embree scene (did you forget to finalize/'commit' the model?)");

    frame->camera = (Camera *)getParam("camera",NULL);
    Assert2(frame->camera,"null camera handle (did you forget to assign a 'camera' parameter to the ray_cast renderer?)");
    frame->_camera = (void*)frame->camera->getIE();
    Assert2(frame->_camera,"invalid model without a ISPC-side camera "
            "(did you forget to finalize/'commit' the camera?)");
    return frame;
  }

  OSP_REGISTER_RENDERER(RayCastRenderer,ray_cast);
  OSP_REGISTER_RENDERER(RayCastRenderer,raycast);
};

