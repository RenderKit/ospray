// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "QuadLight.h"
// embree
#include "embree3/rtcore.h"

#include "lights/QuadLight_ispc.h"

#include "QuadLightShared.h"
#include "common/InstanceShared.h"

namespace ospray {

ispc::Light *QuadLight::createSh(uint32_t, const ispc::Instance *instance) const
{
  ispc::QuadLight *sh = StructSharedCreate<ispc::QuadLight>();
  sh->super.sample = ispc::QuadLight_sample_addr();
  sh->super.eval = ispc::QuadLight_eval_addr();
  sh->super.isVisible = visible;
  sh->super.instance = instance;

  sh->radiance = radiance;

  sh->pre.position = position;
  sh->pre.edge1 = edge1;
  sh->pre.edge2 = edge2;

  intensityDistribution.setSh(sh->intensityDistribution);

  // Enable dynamic runtime instancing or apply static transformation
  if (instance) {
    sh->pre.c0 = intensityDistribution.c0;
    if (instance->motionBlur) {
      sh->super.sample = ispc::QuadLight_sample_instanced_addr();
      sh->super.eval = ispc::QuadLight_eval_instanced_addr();
    } else
      ispc::QuadLight_Transform(sh, instance->xfm, &sh->pre);
  } else {
    const vec3f ndirection = cross(edge2, edge1);
    sh->pre.ppdf = rcp(length(ndirection)); // 1/area
    sh->pre.nnormal = ndirection * sh->pre.ppdf; // normalize
    sh->pre.c90 = normalize(cross(sh->pre.nnormal, intensityDistribution.c0));
    sh->pre.c0 = cross(sh->pre.c90, sh->pre.nnormal);
  }

  return &sh->super;
}

std::string QuadLight::toString() const
{
  return "ospray::QuadLight";
}

void QuadLight::commit()
{
  Light::commit();
  position = getParam<vec3f>("position", vec3f(0.f));
  edge1 = getParam<vec3f>("edge1", vec3f(1.f, 0.f, 0.f));
  edge2 = getParam<vec3f>("edge2", vec3f(0.f, 1.f, 0.f));

  intensityDistribution.c0 = edge2;
  intensityDistribution.readParams(*this);

  queryIntensityQuantityType(intensityDistribution
          ? OSP_INTENSITY_QUANTITY_SCALE
          : OSP_INTENSITY_QUANTITY_RADIANCE);
  processIntensityQuantityType();
}

void QuadLight::processIntensityQuantityType()
{
  const float quadArea = length(cross(edge1, edge2));

  // converting from the chosen intensity quantity type to radiance
  if (intensityDistribution
          ? intensityQuantity == OSP_INTENSITY_QUANTITY_SCALE
          : intensityQuantity == OSP_INTENSITY_QUANTITY_INTENSITY) {
    radiance = coloredIntensity / quadArea;
    return;
  }
  if (!intensityDistribution) {
    if (intensityQuantity == OSP_INTENSITY_QUANTITY_POWER) {
      radiance = coloredIntensity / (M_PI * quadArea);
      return;
    }
    if (intensityQuantity == OSP_INTENSITY_QUANTITY_RADIANCE) {
      radiance = coloredIntensity;
      return;
    }
  }
  postStatusMsg(OSP_LOG_WARNING)
      << toString() << " unsupported 'intensityQuantity' value";
  radiance = vec3f(0.0f);
}

} // namespace ospray
