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
#ifndef OSPRAY_TARGET_SYCL
// ispc exports
#include "geometry/GeometricModel_ispc.h"
#include "render/pathtracer/GeometryLight_ispc.h"
#else
namespace ispc {
float Distribution1D_create(const int size, float *cdf);
int GeometricModel_gatherEmissivePrimIDs(void *_model,
    void *_renderer,
    void *_instance,
    int *primIDs,
    float *distribution,
    float &pdf);
} // namespace ispc
#endif
// ispc shared
#include "GeometryLightShared.h"

namespace ospray {

PathTracerData::PathTracerData(
    const World &world, bool scanForGeometryLights, const Renderer &renderer)
    : AddStructShared(world.getISPCDevice().getDRTDevice())
{
  std::vector<ispc::Light *> lightShs;
  size_t geometryLights{0};

  if (scanForGeometryLights) {
    generateGeometryLights(world, renderer, lightShs);
    scannedForGeometryLights = true;
    geometryLights = lightShs.size();
  }

  if (world.lights) {
    for (auto &&obj : *world.lights) {
      for (uint32_t id = 0; id < obj->getShCount(); id++)
        lightShs.push_back(obj->createSh(id));
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
          lightShs.push_back(obj->createSh(id, inst->getSh()));
      }
    }
  }

  // Prepare light cumulative distribution function
  std::vector<float> lightsCDF(lightShs.size(), 1.f);
  if (lightsCDF.size()) {
    ispc::Distribution1D_create(lightsCDF.size(), lightsCDF.data());
  }

  // Then create shared buffer from the temporary std::vector
  devicert::Device &device = world.getISPCDevice().getDRTDevice();
  lightArray =
      devicert::make_buffer_shared_unique<ispc::Light *>(device, lightShs);
  lightCDFArray = devicert::make_buffer_shared_unique<float>(device, lightsCDF);
  getSh()->lights = lightArray->sharedPtr();
  getSh()->lightsCDF = lightCDFArray->sharedPtr();
  getSh()->numLights = lightArray->size();
  getSh()->numGeoLights = geometryLights;
}

PathTracerData::~PathTracerData()
{
  // Delete all lights structures
  devicert::Device &device = lightArray->getDevice();
  for (ispc::Light *lptr : *lightArray)
    device.free(lptr);
}

ispc::Light *PathTracerData::createGeometryLight(const Instance *instance,
    const GeometricModel *model,
    const int32 numPrimIDs,
    const std::vector<int> &primIDs,
    const std::vector<float> &distribution,
    float pdf)
{
  devicert::Device &device = instance->getISPCDevice().getDRTDevice();
  ispc::GeometryLight *sh = StructSharedCreate<ispc::GeometryLight>(device);

  sh->super.instance = instance->getSh();
#ifndef OSPRAY_TARGET_SYCL
  sh->super.sample = reinterpret_cast<ispc::Light_SampleFunc>(
      ispc::GeometryLight_sample_addr());
#endif
  sh->super.eval = nullptr; // geometry lights are hit and explicitly handled

  sh->model = model->getSh();
  sh->numPrimitives = numPrimIDs;
  sh->pdf = pdf;

  geoLightPrimIDArray.emplace_back(device, primIDs);
  sh->primIDs = geoLightPrimIDArray.back().sharedPtr();
  geoLightDistrArray.emplace_back(device, distribution);
  sh->distribution = geoLightDistrArray.back().sharedPtr();
  return &sh->super;
}

void PathTracerData::generateGeometryLights(const World &world,
    const Renderer &renderer,
    std::vector<ispc::Light *> &lightShs)
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
          int32 numPrimIDs =
              ispc::GeometricModel_gatherEmissivePrimIDs(model->getSh(),
                  renderer.getSh(),
                  instance->getSh(),
                  primIDs.data(),
                  distribution.data(),
                  pdf);

          // check whether the geometry has any emissive primitives
          if (numPrimIDs) {
            lightShs.push_back(createGeometryLight(
                instance, model, numPrimIDs, primIDs, distribution, pdf));
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
