// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "SciVisRenderer.h"
// ospray
#include "common/Data.h"
#include "lights/Light.h"
#include "lights/AmbientLight.h"
// ispc exports
#include "SciVisRenderer_ispc.h"

namespace ospray {
  namespace scivis {

    SciVisRenderer::SciVisRenderer()
    {
      setParam<std::string>("externalNameFromAPI", "scivis");

      ispcEquivalent = ispc::SciVisRenderer_create(this);
    }

    void SciVisRenderer::commit()
    {
      Renderer::commit();

      lightData = (Data*)getParamData("lights");

      lightArray.clear();
      vec3f aoColor = vec3f(0.f);
      bool ambientLights = false;

      if (lightData) {
        for (uint32_t i = 0; i < lightData->size(); i++)
        {
          const Light* const light = ((Light**)lightData->data)[i];
          // extract color from ambient lights and remove them
          const AmbientLight* const ambient = dynamic_cast<const AmbientLight*>(light);
          if (ambient) {
            ambientLights = true;
            aoColor += ambient->getRadiance();
          } else
            lightArray.push_back(light->getIE());
        }
      }

      void **lightPtr = lightArray.empty() ? nullptr : &lightArray[0];

      const bool shadowsEnabled = getParam1i("shadowsEnabled", 0);
      int aoSamples = getParam1i("aoSamples", 0);
      float aoDistance = getParam1f("aoDistance",
                          getParam1f("aoOcclusionDistance"/*old name*/, 1e20f));
      // "aoWeight" is deprecated, use an ambient light instead
      if (!ambientLights)
        aoColor = vec3f(getParam1f("aoWeight", 0.f));
      const bool aoTransparencyEnabled = getParam1i("aoTransparencyEnabled", 0);
      const bool oneSidedLighting = getParam1i("oneSidedLighting", 1);

      ispc::SciVisRenderer_set(getIE(),
                               shadowsEnabled,
                               aoSamples,
                               aoDistance,
                               (ispc::vec3f&)aoColor,
                               aoTransparencyEnabled,
                               lightPtr,
                               lightArray.size(),
                               oneSidedLighting);
    }

    OSP_REGISTER_RENDERER(SciVisRenderer, rt);
    OSP_REGISTER_RENDERER(SciVisRenderer, raytracer);
    OSP_REGISTER_RENDERER(SciVisRenderer, scivis);
    OSP_REGISTER_RENDERER(SciVisRenderer, sv);
    OSP_REGISTER_RENDERER(SciVisRenderer, obj);
    OSP_REGISTER_RENDERER(SciVisRenderer, OBJ);
    OSP_REGISTER_RENDERER(SciVisRenderer, dvr);

  } // ::ospray::scivis
} // ::ospray
