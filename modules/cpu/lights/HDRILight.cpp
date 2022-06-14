// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "HDRILight.h"
// embree
#include "embree3/rtcore.h"
// ispc exports
#include "lights/HDRILight_ispc.h"
#include "lights/Light_ispc.h"
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
  super.sample = ispc::HDRILight_sample_addr();
  super.eval = ispc::HDRILight_eval_addr();

  this->radianceScale = radianceScale;
  if (map) {
    this->map = map;
    this->distribution = distribution;

    this->rcpSize = 1.f / this->map->sizef;
    this->light2world = light2world;

    // Enable dynamic runtime instancing or apply static transformation
    if (instance) {
      if (instance->motionBlur) {
        super.sample = ispc::HDRILight_sample_instanced_addr();
        super.eval = ispc::HDRILight_eval_instanced_addr();
      } else {
        this->light2world = instance->xfm.l * this->light2world;
      }
    }
    world2light = rcp(this->light2world);
  } else {
    super.sample = ispc::HDRILight_sample_dummy_addr();
    super.eval = ispc::Light_eval_addr();
  }
}
} // namespace ispc

namespace ospray {

HDRILight::~HDRILight()
{
  ispc::HDRILight_destroyDistribution(distributionIE);
}

ispc::Light *HDRILight::createSh(uint32_t, const ispc::Instance *instance) const
{
  ispc::HDRILight *sh = StructSharedCreate<ispc::HDRILight>();
  sh->set(visible,
      instance,
      coloredIntensity,
      frame,
      map->getSh(),
      (const ispc::Distribution2D *)distributionIE);
  return &sh->super;
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
  ispc::HDRILight_destroyDistribution(distributionIE);
  distributionIE = nullptr;
  if (map)
    distributionIE = ispc::HDRILight_createDistribution(map->getSh());

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
