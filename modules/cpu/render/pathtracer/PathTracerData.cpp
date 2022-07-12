// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PathTracerData.h"
// ospray
#include "common/Instance.h"
#include "common/World.h"
#include "geometry/GeometricModel.h"
#include "lights/Light.h"
#include "render/Material.h"
#include "render/Renderer.h"
// ispc exports
#include "geometry/GeometricModel_ispc.h"
#include "render/pathtracer/GeometryLight_ispc.h"
// ispc shared
#include "GeometryLightShared.h"

namespace ospray {

PathTracerData::PathTracerData(const World &world,
    bool importanceSampleGeometryLights,
    const Renderer &renderer)
    : AddStructShared(world.getISPCDevice().getIspcrtDevice())
{
  size_t geometryLights{0};

  if (importanceSampleGeometryLights) {
    generateGeometryLights(world, renderer);
    geometryLights = lightViews.size();
  }

  if (world.lights) {
    for (auto &&obj : *world.lights) {
      for (uint32_t id = 0; id < obj->getShCount(); id++)
        lightViews.push_back(obj->createSh(id));
    }
  }

  // Iterate through all world instances
  if (world.instances) {
    for (auto &&inst : *world.instances) {
      // Skip instances without lights
      if (!inst->group->lights)
        continue;

      // Add instance lights to array
      for (auto &&obj : *inst->group->lights) {
        for (uint32_t id = 0; id < obj->getShCount(); id++)
          lightViews.push_back(obj->createSh(id, inst->getSh()));
      }
    }
  }

  // Prepare light cumulative distribution function
  std::vector<float> lightsCDF(lightViews.size(), 1.f);
  ispc::Distribution1D_create(lightsCDF.size(), lightsCDF.data());

  // Retrieve shared memory pointer from each light view and store them
  // in a temporary local std::vector
  std::vector<ispc::Light *> lights(lightViews.size());
  for (uint32_t i = 0; i < lightViews.size(); i++)
    lights[i] = (ispc::Light *)ispcrtSharedPtr(lightViews[i]);

  // Then create shared buffer from the temporary std::vector
  ispcrt::Device &device = world.getISPCDevice().getIspcrtDevice();
  lightArray = make_buffer_shared_unique<ispc::Light *>(device, lights);
  lightCDFArray = make_buffer_shared_unique<float>(device, lightsCDF);
  getSh()->lights = lightArray->sharedPtr();
  getSh()->lightsCDF = lightCDFArray->sharedPtr();
  getSh()->numLights = lights.size();
  getSh()->numGeoLights = geometryLights;
}

PathTracerData::~PathTracerData()
{
  // Delete all lights structures
  for (ISPCRTMemoryView lv : lightViews)
    BufferSharedDelete(lv);
}

ISPCRTMemoryView PathTracerData::createGeometryLight(const Instance *instance,
    const GeometricModel *model,
    const std::vector<int> &primIDs,
    const std::vector<float> &distribution,
    float pdf)
{
  ispcrt::Device &device = instance->getISPCDevice().getIspcrtDevice();
  ISPCRTMemoryView view =
      StructSharedCreate<ispc::GeometryLight>(device.handle());
  ispc::GeometryLight *sh = (ispc::GeometryLight *)ispcrtSharedPtr(view);

  sh->super.instance = instance->getSh();
  sh->super.sample = ispc::GeometryLight_sample_addr();
  sh->super.eval = nullptr; // geometry lights are hit and explicitly handled

  sh->model = model->getSh();
  sh->numPrimitives = primIDs.size();
  sh->pdf = pdf;

  geoLightPrimIDArray.emplace_back(device, primIDs);
  sh->primIDs = geoLightPrimIDArray.back().sharedPtr();
  geoLightDistrArray.emplace_back(device, distribution);
  sh->distribution = geoLightDistrArray.back().sharedPtr();
  return view;
}

void PathTracerData::generateGeometryLights(
    const World &world, const Renderer &renderer)
{
  if (!world.instances)
    return;

  for (auto &&instance : *world.instances) {
    auto geometries = instance->group->geometricModels.ptr;

    if (!geometries)
      return;

    for (auto &&model : *geometries) {
      if (model->hasEmissiveMaterials(renderer.materialData)) {
        if (model->geometry().supportAreaLighting()) {
          std::vector<int> primIDs(model->geometry().numPrimitives());
          std::vector<float> distribution(model->geometry().numPrimitives());
          float pdf = 0.f;
          unsigned int numPrimIDs =
              ispc::GeometricModel_gatherEmissivePrimIDs(model->getSh(),
                  renderer.getSh(),
                  instance->getSh(),
                  primIDs.data(),
                  distribution.data(),
                  pdf);

          // check whether the geometry has any emissive primitives
          if (numPrimIDs) {
            lightViews.push_back(createGeometryLight(
                instance, model, primIDs, distribution, pdf));
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

} // namespace ospray