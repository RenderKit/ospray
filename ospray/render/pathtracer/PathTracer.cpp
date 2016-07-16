// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#undef NDEBUG

#include "PathTracer.h"
// ospray
#include "common/Data.h"
#include "lights/Light.h"
// ispc exports
#include "PathTracer_ispc.h"
// std
#include <map>

namespace ospray {
  PathTracer::PathTracer() : Renderer()
  {
    ispcEquivalent = ispc::PathTracer_create(this);
  }

  /*! \brief create a material of given type */
  Material *PathTracer::createMaterial(const char *type)
  {
    std::string ptType = std::string("PathTracer_")+type;
    Material *material = Material::createMaterial(ptType.c_str());
    if (!material) {
      std::map<std::string,int> numOccurrances;
      const std::string T = type;
      if (numOccurrances[T] == 0)
        std::cout << "#osp:PT: does not know material type '" << type << "'" <<
          " (replacing with OBJMaterial)" << std::endl;
      numOccurrances[T] ++;
      material = Material::createMaterial("PathTracer_OBJMaterial");
      // throw std::runtime_error("invalid path tracer material "+std::string(type));
    }
    material->refInc();
    return material;
  }

  void PathTracer::commit()
  {
    Renderer::commit();

    lightData = (Data*)getParamData("lights");

    lightArray.clear();

    if (lightData)
      for (int i = 0; i < lightData->size(); i++)
        lightArray.push_back(((Light**)lightData->data)[i]->getIE());

    void **lightPtr = lightArray.empty() ? NULL : &lightArray[0];

    const int32 maxDepth = getParam1i("maxDepth", 20);
    const float minContribution = getParam1f("minContribution", 0.01f);
    const float maxRadiance = getParam1f("maxContribution", getParam1f("maxRadiance", inf));
    Texture2D *backplate = (Texture2D*)getParamObject("backplate", NULL);

    ispc::PathTracer_set(getIE(), maxDepth, minContribution, maxRadiance,
                         backplate ? backplate->getIE() : NULL,
                         lightPtr, lightArray.size());
  }

  OSP_REGISTER_RENDERER(PathTracer,pathtracer);
  OSP_REGISTER_RENDERER(PathTracer,pt);

  extern "C" void ospray_init_module_pathtracer()
  {
    printf("Loaded plugin 'pathtracer' ...\n");
  }
};

