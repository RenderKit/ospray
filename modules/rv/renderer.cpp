#include "renderer.h"
#include "../camera/perspectivecamera.h"
// ospray stuff
#include "../../ospray/common/ray.h"
#include "../../ospray/common/model.h"

namespace ospray {
  namespace rv {
    extern "C" void ispc__RVRenderer_renderTile(void *tile, void *camera, void *volume);

    extern void *ispc_camera;
    extern RTCScene embreeScene;

    TileRenderer::RenderJob *RVRenderer::createRenderJob(FrameBuffer *fb)
    {
      TileJob *tj = new TileJob;
      tj->ispc_camera = ispc_camera;
      tj->ispc_scene  = embreeScene;
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


