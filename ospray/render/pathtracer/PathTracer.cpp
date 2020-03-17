// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PathTracer.h"
// ospray
#include "common/Data.h"
#include "common/Instance.h"
#include "geometry/GeometricModel.h"
#include "lights/Light.h"
// ispc exports
#include "GeometryLight_ispc.h"
#include "Material_ispc.h"
#include "PathTracer_ispc.h"
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

void PathTracer::generateGeometryLights(const World &world)
{
  if (!world.instances)
    return;

  for (auto &&instance : *world.instances) {
    auto geometries = instance->group->geometricModels.ptr;

    if (!geometries)
      return;

    affine3f xfm = instance->xfm();

    for (auto &&model : *geometries) {
      if (model->materialData) {
        // check whether the model has any emissive materials
        bool hasEmissive = false;
        for (auto mat : model->ispcMaterialPtrs) {
          if (mat && ispc::PathTraceMaterial_isEmissive(mat)) {
            hasEmissive = true;
            break;
          }
        }

        if (hasEmissive) {
          if (ispc::GeometryLight_isSupported(model->getIE())) {
            void *light =
                ispc::GeometryLight_create(model->getIE(), getIE(), &xfm);

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

void PathTracer::destroyGeometryLights()
{
  for (size_t i = 0; i < geometryLights; i++)
    ispc::GeometryLight_destroy(lightArray[i]);
}

void PathTracer::commit()
{
  Renderer::commit();

  const int32 rouletteDepth = getParam<int>("roulettePathLength", 5);
  const float maxRadiance = getParam<float>("maxContribution", inf);
  vec4f shadowCatcherPlane = getParam<vec4f>("shadowCatcherPlane", vec4f(0.f));
  useGeometryLights = getParam<bool>("geometryLights", true);

  ispc::PathTracer_set(
      getIE(), rouletteDepth, maxRadiance, (ispc::vec4f &)shadowCatcherPlane);
}

void *PathTracer::beginFrame(FrameBuffer *, World *world)
{
  if (!world)
    return nullptr;

  destroyGeometryLights();
  lightArray.clear();
  geometryLights = 0;

  if (useGeometryLights) {
    generateGeometryLights(*world);
    geometryLights = lightArray.size();
  }

  if (world->lights) {
    for (auto &&obj : *world->lights) {
      lightArray.push_back(obj->getIE());
      if (obj->getSecondIE().has_value())
        lightArray.push_back(obj->getSecondIE().value());
    }
  }

  void **lightPtr = lightArray.empty() ? nullptr : &lightArray[0];

  ispc::PathTracer_setLights(
      getIE(), lightPtr, lightArray.size(), geometryLights);
  return nullptr;
}

} // namespace ospray
