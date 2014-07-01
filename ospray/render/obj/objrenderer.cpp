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
// ispc exports
#include "objrenderer_ispc.h"

namespace ospray {
  namespace obj {

    void OBJRenderer::commit()
    {
      world = (Model *)getParamObject("world",NULL);
      Assert2(world,"null world handle (did you forget to assign a "
              "'world' parameter to the ray_cast renderer?)");

      camera = (Camera *)getParamObject("camera",NULL);
      Assert2(camera,"null camera handle (did you forget to assign a "
              "'camera' parameter to the ray_cast renderer?)");

      pointLightData = (Data*)getParamData("pointLights",NULL);

      if (pointLightData && pointLightArray.empty()) {
        for (int i = 0; i < pointLightData->size(); i++) {
          pointLightArray.push_back(((Light**)pointLightData->data)[i]->getIE());
        }
      }

      // pointLightArray = &pointLightArray[0];

      dirLightData = (Data*)getParamData("directionalLights", NULL);

      if (dirLightData && dirLightArray.empty()) {
        for (int i = 0; i < dirLightData->size(); i++) {
          Light *light_i = ((Light**)dirLightData->data)[i];
          PRINT(light_i);
          dirLightArray.push_back(light_i->getIE());
        }
      }

      void **pointLightPtr = pointLightArray.empty() ? NULL : &pointLightArray[0];
      void **dirLightPtr = dirLightArray.empty() ? NULL : &dirLightArray[0];
      ispc::OBJRenderer_set(getIE(),
                            world?world->getIE():NULL,
                            camera?camera->getIE():NULL,
                            pointLightPtr,pointLightArray.size(),
                            dirLightPtr,dirLightArray.size());
    }
    
    OBJRenderer::OBJRenderer()
    {
      ispcEquivalent = ispc::OBJRenderer_create(this);
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
