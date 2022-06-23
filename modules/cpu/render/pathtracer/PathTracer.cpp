// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PathTracer.h"
// ospray
#include "common/Data.h"
#include "common/Instance.h"
#include "geometry/GeometricModel.h"
#include "lights/Light.h"
#include "render/Material.h"
// ispc exports
#include "common/World_ispc.h"
#include "render/pathtracer/GeometryLight_ispc.h"
#include "render/pathtracer/PathTracer_ispc.h"
// std
#include <map>

namespace ospray {

PathTracer::PathTracer()
{
  ispcEquivalent = ispc::PathTracer_create();
}

std::string PathTracer::toString() const
{
  return "ospray::PathTracer";
}

void PathTracer::generateGeometryLights(
    const World &world, std::vector<void *> &lightArray)
{
  if (!world.instances)
    return;

  for (auto &&instance : *world.instances) {
    auto geometries = instance->group->geometricModels.ptr;

    if (!geometries)
      return;

    for (auto &&model : *geometries) {
      if (model->materialData) {
        // check whether the model has any emissive materials
        bool hasEmissive = false;
        for (auto mat : model->ispcMaterialPtrs) {
          if (mat && ((ispc::Material *)mat)->isEmissive()) {
            hasEmissive = true;
            break;
          }
        }
        // Materials from Renderer list
        const auto numRendererMaterials = ispcMaterialPtrs.size();
        if (numRendererMaterials > 0 && model->ispcMaterialPtrs.size() == 0)
          for (auto matIdx : model->materialData->as<uint32_t>())
            if (matIdx < numRendererMaterials
                && ((ispc::Material *)ispcMaterialPtrs[matIdx])->isEmissive()) {
              hasEmissive = true;
              break;
            }

        if (hasEmissive) {
          if (ispc::GeometryLight_isSupported(model->getIE())) {
            void *light = ispc::GeometryLight_create(
                model->getIE(), getIE(), instance->getIE());

            // check whether the geometry has any emissive primitives
            if (light)
              lightArray.push_back(light);
          } else {
            postStatusMsg(OSP_LOG_WARNING)
                << "#osp:pt Geometry " << model->toString()
                << " does not implement area sampling! "
                << "Cannot use importance sampling for that "
                << "geometry with emissive material!";
          }
        }
      }
    }
  }
}

void PathTracer::commit()
{
  Renderer::commit();

  const int32 rouletteDepth = getParam<int>("roulettePathLength", 5);
  const int32 numLightSamples = getParam<int>("lightSamples", -1);
  const float maxRadiance = getParam<float>("maxContribution", inf);
  vec4f shadowCatcherPlane = getParam<vec4f>("shadowCatcherPlane", vec4f(0.f));
  importanceSampleGeometryLights = getParam<bool>("geometryLights", true);
  const bool bgRefraction = getParam<bool>("backgroundRefraction", false);

  ispc::PathTracer_set(getIE(),
      rouletteDepth,
      maxRadiance,
      (ispc::vec4f &)shadowCatcherPlane,
      numLightSamples,
      bgRefraction);
}

void *PathTracer::beginFrame(FrameBuffer *, World *world)
{
  if (!world)
    return nullptr;

  const bool geometryLightListValid =
      importanceSampleGeometryLights == scannedGeometryLights;

  if (world->pathtracerDataValid && geometryLightListValid)
    return nullptr;

  std::vector<void *> lightArray;
  size_t geometryLights{0};

  if (importanceSampleGeometryLights) {
    generateGeometryLights(*world, lightArray);
    geometryLights = lightArray.size();
  }

  if (world->lights) {
    for (auto &&obj : *world->lights) {
      lightArray.push_back(obj->createIE());
      void *secondIE = obj->createSecondIE();
      if (secondIE)
        lightArray.push_back(secondIE);
    }
  }

  // Iterate through all world instances
  if (world->instances) {
    for (auto &&inst : *world->instances) {
      // Skip instances without lights
      if (!inst->group->lights)
        continue;

      // Add instance lights to array
      for (auto &&obj : *inst->group->lights) {
        lightArray.push_back(obj->createIE(inst->getIE()));
        void *secondIE = obj->createSecondIE(inst->getIE());
        if (secondIE)
          lightArray.push_back(secondIE);
      }
    }
  }

  void **lightPtr = lightArray.empty() ? nullptr : &lightArray[0];
  ispc::World_setPathtracerData(
      world->getIE(), lightPtr, lightArray.size(), geometryLights);

  world->pathtracerDataValid = true;
  scannedGeometryLights = importanceSampleGeometryLights;

  return nullptr;
}

} // namespace ospray
