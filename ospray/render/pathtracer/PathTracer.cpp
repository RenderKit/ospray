// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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
    setParam<std::string>("externalNameFromAPI", "pathtracer");

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

  void PathTracer::generateGeometryLights(const Model *const model
      , const affine3f& xfm
      , float *const _areaPDF
      )
  {
    for(size_t i = 0; i < model->geometry.size(); i++) {
      auto &geo = model->geometry[i];
      // recurse instances
      Ref<Instance> inst = geo.dynamicCast<Instance>();
      if (inst) {
        const affine3f instXfm = xfm * inst->xfm;
        generateGeometryLights(inst->instancedScene.ptr, instXfm,
            &(inst->areaPDF[0]));
      } else
        if (geo->materialList) {
          // check whether the geometry has any emissive materials
          bool hasEmissive = false;
          for (auto mat : geo->ispcMaterialPtrs) {
            if (mat && ispc::PathTraceMaterial_isEmissive(mat)) {
              hasEmissive = true;
              break;
            }
          }

          if (hasEmissive) {
            if (ispc::GeometryLight_isSupported(geo->getIE())) {
              const affine3f rcpXfm = rcp(xfm);
              void* light = ispc::GeometryLight_create(geo->getIE()
                  , (const ispc::AffineSpace3f&)xfm
                  , (const ispc::AffineSpace3f&)rcpXfm
                  , _areaPDF+i);

              // check whether the geometry has any emissive primitives
              if (light)
                lightArray.push_back(light);
            } else {
              postStatusMsg(1) << "#osp:pt Geometry " << geo->toString()
                               << " does not implement area sampling! "
                               << "Cannot use importance sampling for that "
                               << "geometry with emissive material!";
            }
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
    geometryLights = 0;

    const bool useGeometryLights = getParam1i("useGeometryLights", true);

    if (model && useGeometryLights) {
      areaPDF.resize(model->geometry.size());
      generateGeometryLights(model, affine3f(one), &areaPDF[0]);
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
        , areaPDF.data()
        );
  }

  OSP_REGISTER_RENDERER(PathTracer,pathtracer);
  OSP_REGISTER_RENDERER(PathTracer,pt);

}// ::ospray
