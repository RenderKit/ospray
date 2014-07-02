// obj
#include "objrenderer.h"
#include "objmaterial.h"
#include "objpointlight.h"
#include "objspotlight.h"
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
      Renderer::commit();

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

      dirLightData = (Data*)getParamData("directionalLights", NULL);

      if (dirLightData && dirLightArray.empty()) {
        for (int i = 0; i < dirLightData->size(); i++) {
          Light *light_i = ((Light**)dirLightData->data)[i];
          PRINT(light_i);
          dirLightArray.push_back(light_i->getIE());
        }
      }

      spotLightData = (Data*)getParamData("spotLights", NULL);
      if( spotLightData && spotLightArray.empty()) {
        for (int i =0; i < spotLightData->size(); i++) {
          spotLightArray.push_back(((Light**)spotLightData->data)[i]->getIE());
        }
      }
      
      void **pointLightPtr = pointLightArray.empty() ? NULL : &pointLightArray[0];
      void **dirLightPtr = dirLightArray.empty() ? NULL : &dirLightArray[0];
      void **spotLightPtr = spotLightArray.empty() ? NULL : &spotLightArray[0];

      vec3f bgColor;
      bgColor = getParam3f("bgColor", vec3f(0));

      bool shadowsEnabled = bool(getParam1i("shadowsEnabled", 1));

      ispc::OBJRenderer_set(getIE(),
                            world->getIE(),
                            camera->getIE(),
                            (ispc::vec3f&)bgColor,
                            shadowsEnabled,
                            pointLightPtr, pointLightArray.size(),
                            dirLightPtr,   dirLightArray.size(),
                            spotLightPtr,  spotLightArray.size());
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
      Light *light = NULL;

      if (strcmp("PointLight", type) == 0) {
        light = new OBJPointLight;
      } else if (strcmp("SpotLight", type) == 0) {
        light = new OBJSpotLight;
      }

      return light;
    }

    OSP_REGISTER_RENDERER(OBJRenderer,OBJ);
    OSP_REGISTER_RENDERER(OBJRenderer,obj);
  }
}
