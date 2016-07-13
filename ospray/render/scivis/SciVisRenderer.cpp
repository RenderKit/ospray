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

// obj
#include "SciVisRenderer.h"
#include "SciVisMaterial.h"
// ospray
#include "common/Data.h"
#include "lights/Light.h"
//sys
#include <vector>
// ispc exports
#include "SciVisRenderer_ispc.h"

namespace ospray {
  namespace scivis {

    void SciVisRenderer::commit()
    {
      Renderer::commit();

      lightData = (Data*)getParamData("lights");

      lightArray.clear();

      if (lightData) {
        for (int i = 0; i < lightData->size(); i++)
          lightArray.push_back(((Light**)lightData->data)[i]->getIE());
      }

      void **lightPtr = lightArray.empty() ? NULL : &lightArray[0];

      const bool shadowsEnabled = getParam1i("shadowsEnabled", 0);

      const int32 maxDepth = getParam1i("maxDepth", 10);

      int   numAOSamples = getParam1i("aoSamples", 0);
      float rayLength    = getParam1f("aoOcclusionDistance", 1e20f);
      float aoWeight     = getParam1f("aoWeight", 0.25f);

      ispc::SciVisRenderer_set(getIE(),
                               shadowsEnabled,
                               maxDepth,
                               numAOSamples,
                               rayLength,
                               aoWeight,
                               lightPtr,
                               lightArray.size());
    }

    SciVisRenderer::SciVisRenderer()
    {
      ispcEquivalent = ispc::SciVisRenderer_create(this);
    }

    /*! \brief create a material of given type */
    Material *SciVisRenderer::createMaterial(const char *type)
    {
      return new SciVisMaterial;
    }

    OSP_REGISTER_RENDERER(SciVisRenderer, raytracer);
    OSP_REGISTER_RENDERER(SciVisRenderer, rt);
    OSP_REGISTER_RENDERER(SciVisRenderer, scivis);
    OSP_REGISTER_RENDERER(SciVisRenderer, sv);
    OSP_REGISTER_RENDERER(SciVisRenderer, obj);
    OSP_REGISTER_RENDERER(SciVisRenderer, OBJ);

  } // ::ospray::scivis
} // ::ospray
