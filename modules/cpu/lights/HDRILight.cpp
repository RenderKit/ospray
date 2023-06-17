// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "HDRILight.h"
#include "common/StructShared.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc exports
#include "lights/HDRILight_ispc.h"
#include "lights/Light_ispc.h"
#else
namespace ispc {
void HDRILight_initDistribution(const void *map, void *distribution);
}
#endif
// ispc shared
#include "HDRILightShared.h"
#include "common/InstanceShared.h"

namespace ispc {
void HDRILight::set(bool isVisible,
    const Instance *instance,
    const vec3f &radianceScale,
    const linear3f &light2world,
    const Texture2D *map,
    const Distribution2D *distribution)
{
  super.isVisible = isVisible;
  super.instance = instance;
#ifndef OSPRAY_TARGET_SYCL
  super.sample =
      reinterpret_cast<ispc::Light_SampleFunc>(ispc::HDRILight_sample_addr());
  super.eval =
      reinterpret_cast<ispc::Light_EvalFunc>(ispc::HDRILight_eval_addr());
#endif

  this->radianceScale = radianceScale;
  if (map) {
    this->map = map;
    this->distribution = distribution;

    this->rcpSize = 1.f / this->map->sizef;
    this->light2world = light2world;

    // Enable dynamic runtime instancing or apply static transformation
    if (instance) {
      if (instance->motionBlur) {
#ifndef OSPRAY_TARGET_SYCL
        super.sample = reinterpret_cast<ispc::Light_SampleFunc>(
            ispc::HDRILight_sample_instanced_addr());
        super.eval = reinterpret_cast<ispc::Light_EvalFunc>(
            ispc::HDRILight_eval_instanced_addr());
#endif
      } else {
        this->light2world = instance->xfm.l * this->light2world;
      }
    }
    world2light = rcp(this->light2world);
  } else {
#ifndef OSPRAY_TARGET_SYCL
    super.sample = reinterpret_cast<ispc::Light_SampleFunc>(
        ispc::HDRILight_sample_dummy_addr());
    super.eval =
        reinterpret_cast<ispc::Light_EvalFunc>(ispc::Light_eval_addr());
#endif
  }
}
} // namespace ispc

namespace ospray {

ISPCRTMemoryView HDRILight::createSh(
    uint32_t, const ispc::Instance *instance) const
{
  ISPCRTMemoryView view = StructSharedCreate<ispc::HDRILight>(
      getISPCDevice().getIspcrtContext().handle());
  ispc::HDRILight *sh = (ispc::HDRILight *)ispcrtSharedPtr(view);
  sh->set(visible,
      instance,
      coloredIntensity,
      frame,
      map->getSh(),
      distribution->getSh());
  return view;
}

std::string HDRILight::toString() const
{
  return "ospray::HDRILight";
}

void HDRILight::commit()
{
  Light::commit();
  const vec3f up = getParam<vec3f>("up", vec3f(0.f, 1.f, 0.f));
  const vec3f dir = getParam<vec3f>("direction", vec3f(0.f, 0.f, 1.f));
  map = (Texture2D *)getParamObject("map", nullptr);

  // recreate distribution
  distribution = nullptr;
  if (map) {
    distribution = new Distribution2D(map->getSh()->size, getISPCDevice());
    // Release extra local ref
    distribution->refDec();

    ispc::HDRILight_initDistribution(map->getSh(), distribution->getSh());
  }

  frame.vx = normalize(-dir);
  frame.vy = normalize(cross(frame.vx, up));
  frame.vz = cross(frame.vx, frame.vy);

  queryIntensityQuantityType(OSP_INTENSITY_QUANTITY_SCALE);
  processIntensityQuantityType();
}

void HDRILight::processIntensityQuantityType()
{
  // validate the correctness of the light quantity type
  if (intensityQuantity != OSP_INTENSITY_QUANTITY_SCALE
      && intensityQuantity != OSP_INTENSITY_QUANTITY_RADIANCE) {
    postStatusMsg(OSP_LOG_WARNING)
        << toString() << " unsupported 'intensityQuantity' value";
    coloredIntensity = vec3f(0.0f);
  }
}

} // namespace ospray
