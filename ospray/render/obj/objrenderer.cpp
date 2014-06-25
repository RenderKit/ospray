// obj
#include "objrenderer.h"
#include "objmaterial.h"
#include "objpointlight.h"
// ospray
#include "ospray/common/model.h"
#include "ospray/common/data.h"
#include "ospray/camera/camera.h"
//embree
#include "embree2/rtcore.h"
//sys
#include <vector>

namespace ospray {
  namespace obj {

    extern "C" void ispc__OBJRenderer_renderTile( void *tile,
                                                  void *camera,
                                                  void *model,
                                                  int num_point_lights,
                                                  void **_point_lights,
                                                  int num_dir_lights,
                                                  void **_dir_lights);

    void OBJRenderer::RenderTask::renderTile(Tile &tile)
    {
      ispc__OBJRenderer_renderTile(&tile,
                                   camera->getIE(),
                                   world->getIE(),
                                   numPointLights,
                                   pointLightArray,
                                   numDirLights,
                                   dirLightArray);
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

      frame->pointLightData = (Data*)getParamData("pointLights",NULL);

      if (frame->pointLightData && pointLightArray.empty()) {
        for (int i = 0; i < frame->pointLightData->size(); i++) {
          pointLightArray.push_back(((Light**)frame->pointLightData->data)[i]->getIE());
        }
      }

      frame->pointLightArray = &pointLightArray[0];

      frame->numPointLights = pointLightArray.size();

      frame->dirLightData = (Data*)getParamData("directionalLights", NULL);

      if (frame->dirLightData && dirLightArray.empty()) {
        for (int i = 0; i < frame->dirLightData->size(); i++) {
          dirLightArray.push_back(((Light**)frame->dirLightData->data)[i]->getIE());
        }
      }

      frame->dirLightArray = &dirLightArray[0];

      frame->numDirLights = dirLightArray.size();

      return frame;
    }
    
    /*! \brief create a material of given type */
    Material *OBJRenderer::createMaterial(const char *type)
    {
      Material *mat = new OBJMaterial;
      return mat;
    }

    /*! \brief create a light of given type */
    Light *OBJRenderer::createLight(const char *type)
    {
      if(!strcmp("PointLight", type)) {
        Light *light = new OBJPointLight;
        return light;
      }
      return NULL;
    }

    OSP_REGISTER_RENDERER(OBJRenderer,OBJ);
    OSP_REGISTER_RENDERER(OBJRenderer,obj);
  }
}
