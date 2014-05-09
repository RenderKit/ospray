#undef NDEBUG

// ospray
#include "renderer.h"
#include "renderer_ispc.h"
#include "ospray/camera/perspectivecamera.h"
#include "common/data.h"
// embree
#include "common/sys/sync/atomic.h"
#include "model.h"

namespace ospray {
  // extern "C" void ispc__TachyonRenderer_renderTile(void *tile, void *camera, void *model);

  void TachyonRenderer::RenderTask::renderTile(Tile &tile)
  {
    ispc::TachyonRenderer_renderTile(&tile,
                                     camera->getIE(),world->getIE(),
                                     textureData->data,
                                     pointLightArray,numPointLights,
                                     dirLightArray,numDirLights
                                     );
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

    frame->pointLightData = (Data*)model->getParamObject("pointLights",NULL);
    frame->pointLightArray
      = frame->pointLightData
      ? frame->pointLightData->data
      : NULL;
    frame->numPointLights
      = frame->pointLightData
      ? frame->pointLightData->size()/sizeof(ospray::tachyon::PointLight)
      : 0;


    frame->dirLightData = (Data*)model->getParamObject("dirLights",NULL);
    frame->dirLightArray
      = frame->dirLightData
      ? frame->dirLightData->data
      : NULL;
    frame->numDirLights
      = frame->dirLightData
      ? frame->dirLightData->size()/sizeof(ospray::tachyon::DirLight)
      : 0;


    frame->camera = (Camera *)getParamObject("camera",NULL);
    return frame;
  }

  OSP_REGISTER_RENDERER(TachyonRenderer,tachyon);
};

