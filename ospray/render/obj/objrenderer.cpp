// obj
#include "objrenderer.h"
#include "objmaterial.h"
// ospray
#include "ospray/common/model.h"
#include "ospray/common/data.h"
#include "ospray/camera/camera.h"
//embree
#include "embree2/rtcore.h"

namespace ospray {
  namespace obj {

    extern "C" void ispc__OBJRenderer_renderTile(void *tile, void *camera, void *model, void *textureArray);

    void OBJRenderer::RenderTask::renderTile(Tile &tile)
    {
      ispc__OBJRenderer_renderTile(&tile,
                                   camera->getIE(),
                                   world->getIE(),
                                   NULL/*textureData->data*/); //textures NYI
    }
    
    TileRenderer::RenderJob *OBJRenderer::createRenderJob(FrameBuffer *fb)
    {
      RenderTask *frame = new RenderTask;
      frame->world = (Model *)getParamObject("world",NULL);
      Assert2(frame->world,"null world handle (did you forget to assign a "
              "'world' parameter to the ray_cast renderer?)");

      frame->camera = (Camera *)getParamObject("camera",NULL);
      Assert2(frame->camera,"null camera handle (did you forget to assign a "
              "'camera' parameter to the ray_cast renderer?)");

      frame->textureData = (Data*)frame->world->getParamObject("textureArray", NULL);

      return frame;
    }
    
    /*! \brief create a material of given type */
    Material *OBJRenderer::createMaterial(const char *type)
    {
      return new OBJMaterial;
    }

    OSP_REGISTER_RENDERER(OBJRenderer,OBJ);
    OSP_REGISTER_RENDERER(OBJRenderer,obj);
  }
}
