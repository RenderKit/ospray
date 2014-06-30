#undef NDEBUG

// ospray
#include "tachyonRenderer.h"
#include "renderer_ispc.h"
#include "ospray/camera/perspectivecamera.h"
#include "common/data.h"
// tachyon
#include "model.h"
// ispc imports
#include "tachyonRenderer_ispc.h"

namespace ospray {
  // extern "C" void ispc__TachyonRenderer_renderTile(void *tile, void *camera, void *model);

  // void TachyonRenderer::RenderTask::renderTile(Tile &tile)
  // {
  //   ispc::TachyonRenderer_renderTile(&tile,
  //                                    camera->getIE(),world->getIE(),
  //                                    textureData->data,
  //                                    pointLightArray,numPointLights,
  //                                    dirLightArray,numDirLights,doShadows
  //                                    );
  // }
  TachyonRenderer::TachyonRenderer()
  {
    this->ispcEquivalent = ispc::TachyonRenderer_create(this);
  }

  void TachyonRenderer::commit()
  {
    model = (Model *)getParamObject("world",NULL);
    model = (Model *)getParamObject("model",model);
    if (!model)
      throw std::runtime_error("#osp:tachyon::renderer: no model specified");

    doShadows = getParam1i("do_shadows",0);
    
    float lightScale = getParam1f("lightScale",1.f);

    textureData = (Data*)model->getParamObject("textureArray",NULL);
    if (!textureData)
      throw std::runtime_error("#osp:tachyon::renderer: no texture data specified");

    pointLightData = (Data*)model->getParamObject("pointLights",NULL);
    pointLightArray
      = pointLightData
      ? pointLightData->data
      : NULL;
    numPointLights
      = pointLightData
      ? pointLightData->size()/sizeof(ospray::tachyon::PointLight)
      : 0;

    dirLightData = (Data*)model->getParamObject("dirLights",NULL);
    dirLightArray
      = dirLightData
      ? dirLightData->data
      : NULL;
    numDirLights
      = dirLightData
      ? dirLightData->size()/sizeof(ospray::tachyon::DirLight)
      : 0;

    bool doShadows = getParam1i("do_shadows",0);

    camera = (Camera *)getParamObject("camera",NULL);
    ispc::TachyonRenderer_set(getIE(),
                              model->getIE(),
                              camera->getIE(),
                              textureData->data,
                              pointLightArray,numPointLights,
                              dirLightArray,numDirLights,
                              doShadows,
                              lightScale);
  }

  // TileRenderer::RenderJob *TachyonRenderer::createRenderJob(FrameBuffer *fb)
  // {
  //   /*! iw - actually, shoudl do all this parsing in 'commit', not in createrenderjob */

  //   RenderTask *frame = new RenderTask;
  //   // should actually do that in 'commit', not ever frame ...
  //   Model *model = (Model *)getParamObject("world",NULL);
  //   model = (Model *)getParamObject("model",model);
  //   if (!model)
  //     throw std::runtime_error("#osp:tachyon::renderer: no model specified");
  //   frame->world = model;

  //   frame->doShadows = getParam1i("do_shadows",0);

  //   frame->textureData = (Data*)model->getParamObject("textureArray",NULL);
  //   if (!frame->textureData)
  //     throw std::runtime_error("#osp:tachyon::renderer: no texture data specified");

  //   frame->pointLightData = (Data*)model->getParamObject("pointLights",NULL);
  //   frame->pointLightArray
  //     = frame->pointLightData
  //     ? frame->pointLightData->data
  //     : NULL;
  //   frame->numPointLights
  //     = frame->pointLightData
  //     ? frame->pointLightData->size()/sizeof(ospray::tachyon::PointLight)
  //     : 0;


  //   frame->dirLightData = (Data*)model->getParamObject("dirLights",NULL);
  //   frame->dirLightArray
  //     = frame->dirLightData
  //     ? frame->dirLightData->data
  //     : NULL;
  //   frame->numDirLights
  //     = frame->dirLightData
  //     ? frame->dirLightData->size()/sizeof(ospray::tachyon::DirLight)
  //     : 0;


  //   frame->camera = (Camera *)getParamObject("camera",NULL);
  //   return frame;
  // }

  OSP_REGISTER_RENDERER(TachyonRenderer,tachyon);
};

