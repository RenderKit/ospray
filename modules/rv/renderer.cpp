#include "renderer.h"
#include "../camera/perspectivecamera.h"
// ospray stuff
#include "../../ospray/common/ray.h"
#include "../../ospray/common/model.h"

namespace ospray {
  namespace rv {
    extern "C" void ispc__RVRenderer_renderTile(void *tile, void *camera, void *volume);

    TileRenderer::RenderJob *RVRenderer::createRenderJob(FrameBuffer *fb)
    {
      Assert(fb && "null frame buffer handle");
      void *_fb = fb->getIE();
      Assert(_fb && "invalid host-side-only frame buffer");

      Model *world = (Model *)getParam("world",NULL);
      Assert2(world,"null world handle (did you forget to assign a 'world' parameter to the ray_cast renderer?)");
      void *_scene = (void*)world->eScene;
      Assert2(_scene,"invalid model without an embree scene (did you forget to finalize/'commit' the model?)");

      Camera *camera = (Camera *)getParam("camera",NULL);
      Assert(camera && "null camera handle in 'rv' renderer "
             "(did you forget to assign a 'camera' parameter to the renderer?)");
      void *_camera = (void*)camera->getIE();
      Assert(_camera && "invalid camera without a ISPC-side camera "
             "(did you forget to finalize/'commit' the camera?)");

      TileJob *tj = new TileJob;
      tj->ispc_camera = _camera;
      tj->ispc_scene  = _scene;
      return tj;
    }

    void RVRenderer::TileJob::renderTile(Tile &tile)
    {
      ispc__RVRenderer_renderTile(&tile,ispc_camera,ispc_scene);
    }

    OSP_REGISTER_RENDERER(RVRenderer,rv);

    extern "C" void ospray_init_module_rv() 
    {
      printf("#osp:rv: module loaded...\n");
    }
  }
}


