// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PathTracer.h"
// ospray
#include "camera/Camera.h"
#include "common/Data.h"
#include "common/Instance.h"
#include "common/World.h"
#include "fb/FrameBuffer.h"
#include "geometry/GeometricModel.h"
#include "lights/Light.h"
#include "render/Material.h"
// ispc exports
#include "geometry/GeometricModel_ispc.h"
#include "render/bsdfs/MicrofacetAlbedoTables_ispc.h"
#include "render/pathtracer/PathTracer_ispc.h"
// ispc shared
#include "render/pathtracer/GeometryLightShared.h"
// std
#include <map>

namespace ospray {

std::string PathTracer::toString() const
{
  return "ospray::PathTracer";
}

void PathTracer::generateGeometryLights(
    const World &world, std::vector<ispc::Light *> &lightArray)
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
          if (mat && mat->isEmissive()) {
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
          if (model->geometry().supportAreaLighting()) {
            std::vector<int> primIDs(model->geometry().numPrimitives());
            std::vector<float> distribution(model->geometry().numPrimitives());
            float pdf = 0.f;
            unsigned int numPrimIDs =
                ispc::GeometricModel_gatherEmissivePrimIDs(model->getSh(),
                    getSh(),
                    instance->getSh(),
                    primIDs.data(),
                    distribution.data(),
                    pdf);

            // check whether the geometry has any emissive primitives
            if (numPrimIDs) {
              ispc::GeometryLight *light =
                  StructSharedCreate<ispc::GeometryLight>();
              light->create(instance->getSh(),
                  model->getSh(),
                  numPrimIDs,
                  primIDs.data(),
                  distribution.data(),
                  pdf);
              lightArray.push_back(&light->super);
            }
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

  getSh()->rouletteDepth = getParam<int>("roulettePathLength", 5);
  getSh()->maxRadiance = getParam<float>("maxContribution", inf);
  getSh()->numLightSamples = getParam<int>("lightSamples", -1);

  // Set shadow catcher plane
  const vec4f shadowCatcherPlane =
      getParam<vec4f>("shadowCatcherPlane", vec4f(0.f));
  const vec3f normal = vec3f(shadowCatcherPlane);
  const float l = length(normal);
  getSh()->shadowCatcher = l > 0.f;
  const float rl = rcp(l);
  getSh()->shadowCatcherPlane = vec4f(normal * rl, shadowCatcherPlane.w * rl);

  importanceSampleGeometryLights = getParam<bool>("geometryLights", true);
  getSh()->backgroundRefraction = getParam<bool>("backgroundRefraction", false);

  ispc::precomputeMicrofacetAlbedoTables();
}

void *PathTracer::beginFrame(FrameBuffer *, World *world)
{
  if (!world)
    return nullptr;

  const bool geometryLightListValid =
      importanceSampleGeometryLights == scannedGeometryLights;

  if (world->pathtracerDataValid && geometryLightListValid)
    return nullptr;

  std::vector<ispc::Light *> lightArray;
  size_t geometryLights{0};

  if (importanceSampleGeometryLights) {
    generateGeometryLights(*world, lightArray);
    geometryLights = lightArray.size();
  }

  if (world->lights) {
    for (auto &&obj : *world->lights) {
      for (uint32_t id = 0; id < obj->getShCount(); id++)
        lightArray.push_back(obj->createSh(id));
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
        for (uint32_t id = 0; id < obj->getShCount(); id++)
          lightArray.push_back(obj->createSh(id, inst->getSh()));
      }
    }
  }

  // Prepare light cumulative distribution function
  std::vector<float> lightsCDF(lightArray.size(), 1.f);
  ispc::Distribution1D_create(lightsCDF.size(), lightsCDF.data());

  // Prepare pathtracer data structure
  ispc::PathtracerData &pd = world->getSh()->pathtracerData;
  pd.destroy();
  pd.create(
      lightArray.data(), lightArray.size(), geometryLights, lightsCDF.data());

  world->pathtracerDataValid = true;
  scannedGeometryLights = importanceSampleGeometryLights;

  return nullptr;
}

void PathTracer::renderTasks(FrameBuffer *fb,
    Camera *camera,
    World *world,
    void *perFrameData,
    const utility::ArrayView<uint32_t> &taskIDs) const
{
  ispc::PathTracer_renderTasks(getSh(),
      fb->getSh(),
      camera->getSh(),
      world->getSh(),
      perFrameData,
      taskIDs.data(),
      taskIDs.size());
}

} // namespace ospray
