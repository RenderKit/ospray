// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// obj
#include "OBJRenderer.h"
#include "OBJMaterial.h"
// ospray
#include "ospray/common/Model.h"
#include "ospray/common/Data.h"
#include "ospray/camera/Camera.h"
#include "ospray/lights/Light.h"
//embree
#include "embree2/rtcore.h"
//sys
#include <vector>
// ispc exports
#include "OBJRenderer_ispc.h"

namespace ospray {
  namespace obj {

    void OBJRenderer::commit()
    {
      Renderer::commit();

      world = (Model *)getParamObject("world",NULL);
      world = (Model *)getParamObject("model",world);
      camera = (Camera *)getParamObject("camera",NULL);

      lightData = (Data*)getParamData("lights",NULL);

      if (lightData && lightArray.empty())
        for (int i = 0; i < lightData->size(); i++)
          lightArray.push_back(((Light**)lightData->data)[i]->getIE());

      void **lightPtr = lightArray.empty() ? NULL : &lightArray[0];

      vec3f bgColor;
      bgColor = getParam3f("bgColor", vec3f(1.f));

      bool shadowsEnabled = bool(getParam1i("shadowsEnabled", 1));
      ispc::OBJRenderer_set(getIE(),
                            world?world->getIE():NULL,
                            camera?camera->getIE():NULL,
                            (ispc::vec3f&)bgColor,
                            shadowsEnabled,
                            lightPtr, lightArray.size());
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

    OSP_REGISTER_RENDERER(OBJRenderer,OBJ);
    OSP_REGISTER_RENDERER(OBJRenderer,obj);
  } // ::ospray::obj
} // ::ospray
