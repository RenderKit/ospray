// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "geometry/Instance.h"
// ispc exports
#include "PathTracer_ispc.h"
#include "Material_ispc.h"
#include "GeometryLight_ispc.h"
// std
#include <map>

namespace ospray {

  PathTracer::PathTracer()
  {
    ispcEquivalent = ispc::PathTracer_create(this);
  }

  PathTracer::~PathTracer()
  {
    destroyGeometryLights();
  }

  std::string PathTracer::toString() const
  {
    return "ospray::PathTracer";
  }

  Material *PathTracer::createMaterial(const char *type)
  {
    std::string ptType = std::string("PathTracer_")+type;
    Material *material = Material::createMaterial(ptType.c_str());
    if (!material) {
      std::map<std::string,int> numOccurrances;
      const std::string T = type;
      if (numOccurrances[T] == 0) {
        postStatusMsg() << "#osp:PT: does not know material type '" << type
                        << "'" << " (replacing with OBJMaterial)";
      }
      numOccurrances[T]++;
      material = Material::createMaterial("PathTracer_OBJMaterial");
    }
    material->refInc();
    return material;
  }


  void PathTracer::generateGeometryLights(const Model *const model
      , const affine3f& xfm
      , const affine3f& rcp_xfm
      , float *const _areaPDF
      )
  {
    for(size_t i = 0; i < model->geometry.size(); i++) {
      auto &geo = model->geometry[i];
      // recurse instances
      Ref<Instance> inst = geo.dynamicCast<Instance>();
      if (inst) {
        const affine3f instXfm = xfm * inst->xfm;
        const affine3f rcpXfm = rcp(instXfm);
        generateGeometryLights(inst->instancedScene.ptr, instXfm, rcpXfm,
            &(inst->areaPDF[0]));
      } else
        if (geo->material && geo->material->getIE()
            && ispc::PathTraceMaterial_isEmissive(geo->material->getIE())) {
          void* light = ispc::GeometryLight_create(geo->getIE()
              , (const ispc::AffineSpace3f&)xfm
              , (const ispc::AffineSpace3f&)rcp_xfm
              , _areaPDF+i);

          if (light)
            lightArray.push_back(light);
          else {
            postStatusMsg(1) << "#osp:pt Geometry " << geo->toString()
                             << " does not implement area sampling! "
                             << "Cannot use importance sampling for that "
                             << "geometry with emissive material!";
          }
        }
    }
  }

  void PathTracer::destroyGeometryLights()
  {
    for (size_t i = 0; i < geometryLights; i++)
      ispc::GeometryLight_destroy(lightArray[i]);
  }

  void PathTracer::commit()
  {
    Renderer::commit();

    destroyGeometryLights();
    lightArray.clear();

    if (model) {
      areaPDF.resize(model->geometry.size());
      generateGeometryLights(model, affine3f(one), affine3f(one), &areaPDF[0]);
      geometryLights = lightArray.size();
    }

    lightData = (Data*)getParamData("lights");
    if (lightData) {
      for (uint32_t i = 0; i < lightData->size(); i++)
        lightArray.push_back(((Light**)lightData->data)[i]->getIE());
    }

    void **lightPtr = lightArray.empty() ? nullptr : &lightArray[0];

    const int32 rouletteDepth = getParam1i("rouletteDepth", 5);
    const float maxRadiance = getParam1f("maxContribution",
                                         getParam1f("maxRadiance", inf));
    Texture2D *backplate = (Texture2D*)getParamObject("backplate", nullptr);
    vec4f shadowCatcherPlane = getParam4f("shadowCatcherPlane", vec4f(0.f));

    ispc::PathTracer_set(getIE()
        , rouletteDepth
        , maxRadiance
        , backplate ? backplate->getIE() : nullptr
        , (ispc::vec4f&)shadowCatcherPlane
        , lightPtr
        , lightArray.size()
        , geometryLights
        , &areaPDF[0]
        );
  }

  OSP_REGISTER_RENDERER(PathTracer,pathtracer);
  OSP_REGISTER_RENDERER(PathTracer,pt);

}// ::ospray
