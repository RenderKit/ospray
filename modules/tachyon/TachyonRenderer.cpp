#undef NDEBUG

// ospray
#include "TachyonRenderer.h"
#include "ospray/camera/PerspectiveCamera.h"
#include "common/Data.h"
// tachyon
#include "Model.h"
// ispc imports
#include "TachyonRenderer_ispc.h"

namespace ospray {

  TachyonRenderer::TachyonRenderer()
  {
    this->ispcEquivalent = ispc::TachyonRenderer_create(this);
  }

  void TachyonRenderer::commit()
  {
    Renderer::commit();

    model = (Model *)getParamObject("world",NULL);
    model = (Model *)getParamObject("model",model);

    doShadows = getParam1i("do_shadows",0);
    
    float lightScale = getParam1f("lightScale",1.f);

    textureData = (Data*)(model ? model->getParamObject("textureArray",NULL) : NULL);
    if (!textureData)
      throw std::runtime_error("#osp:tachyon::renderer: no texture data specified");

    pointLightData = (Data*)(model ? model->getParamObject("pointLights",NULL) : NULL);
    pointLightArray
      = pointLightData
      ? pointLightData->data
      : NULL;
    numPointLights
      = pointLightData
      ? pointLightData->size()/sizeof(ospray::tachyon::PointLight)
      : 0;

    dirLightData = (Data*)(model ? model->getParamObject("dirLights",NULL) : NULL);
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
                              model?model->getIE():NULL,
                              camera?camera->getIE():NULL,
                              textureData?textureData->data:NULL,
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

  extern "C" void ospray_init_module_tachyon() 
  {
    printf("Loaded plugin 'pathtracer' ...\n");
  }
};

