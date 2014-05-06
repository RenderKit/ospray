#undef NDEBUG

// ospray
#include "renderer.h"
#include "renderer_ispc.h"
#include "ospray/camera/perspectivecamera.h"
#include "common/data.h"
// embree
#include "common/sys/sync/atomic.h"

namespace ospray {
  // extern "C" void ispc__TachyonRenderer_renderTile(void *tile, void *camera, void *model);

  void TachyonRenderer::RenderTask::renderTile(Tile &tile)
  {
    ispc::TachyonRenderer_renderTile(&tile,
                                     camera->getIE(),world->getIE(),
                                     textureData->getIE());
  }

  TileRenderer::RenderJob *TachyonRenderer::createRenderJob(FrameBuffer *fb)
  {
    RenderTask *frame = new RenderTask;
    // should actually do that in 'commit', not ever frame ...
    Model *model = (Model *)getParamObject("world",NULL);
    model = (Model *)getParamObject("model",model);
    if (!model)
      throw std::runtime_error("#osp:tachyon::renderer: no model specified");
    frame->world = model;

    frame->textureData = (Data*)model->getParamObject("textureArray",NULL);
    if (!frame->textureData)
      throw std::runtime_error("#osp:tachyon::renderer: no texture data specified");

    frame->camera = (Camera *)getParamObject("camera",NULL);
    return frame;
  }

  OSP_REGISTER_RENDERER(TachyonRenderer,tachyon);
};

