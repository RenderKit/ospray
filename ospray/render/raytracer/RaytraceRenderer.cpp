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
#include "RaytraceRenderer.h"
#include "RaytraceMaterial.h"
// ospray
#include "ospray/common/Data.h"
#include "ospray/lights/Light.h"
//sys
#include <vector>
// ispc exports
#include "RaytraceRenderer_ispc.h"

namespace ospray {
  namespace raytracer {

    void RaytraceRenderer::commit()
    {
      Renderer::commit();

      lightData = (Data*)getParamData("lights");

      lightArray.clear();

      if (lightData) {
        for (int i = 0; i < lightData->size(); i++)
          lightArray.push_back(((Light**)lightData->data)[i]->getIE());
      }

      void **lightPtr = lightArray.empty() ? NULL : &lightArray[0];

      vec3f bgColor;
      bgColor = getParam3f("bgColor", vec3f(1.f));

      const bool shadowsEnabled = bool(getParam1i("shadowsEnabled", 1));

      const int32 maxDepth = getParam1i("maxDepth", 10);

      int   numAOSamples = getParam1i("aoSamples", 4); // number of AO rays per pixel sample
      float rayLength    = getParam1f("aoOcclusionDistance", 1e20f);
      float aoWeight     = getParam1f("aoWeight", 0.25f);

      ispc::RaytraceRenderer_set(getIE(),
                                 (ispc::vec3f&)bgColor,
                                 shadowsEnabled,
                                 maxDepth,
                                 numAOSamples,
                                 rayLength,
                                 aoWeight,
                                 lightPtr,
                                 lightArray.size());
    }

    RaytraceRenderer::RaytraceRenderer()
    {
      ispcEquivalent = ispc::RaytraceRenderer_create(this);
    }

    /*! \brief create a material of given type */
    Material *RaytraceRenderer::createMaterial(const char *type)
    {
      Material *mat = new RaytraceMaterial;
      return mat;
    }

    OSP_REGISTER_RENDERER(RaytraceRenderer, raytracer);
    OSP_REGISTER_RENDERER(RaytraceRenderer, rt);
    OSP_REGISTER_RENDERER(RaytraceRenderer, scivis);

  } // ::ospray::obj
} // ::ospray
