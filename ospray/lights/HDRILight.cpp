// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "HDRILight.h"
#include "HDRILight_ispc.h"

namespace ospray {

HDRILight::HDRILight()
{
  ispcEquivalent = ispc::HDRILight_create();
}

HDRILight::~HDRILight()
{
  ispc::HDRILight_destroy(getIE());
  ispcEquivalent = nullptr;
}

std::string HDRILight::toString() const
{
  return "ospray::HDRILight";
}

void HDRILight::commit()
{
  Light::commit();
  up = getParam<vec3f>("up", vec3f(0.f, 1.f, 0.f));
  dir = getParam<vec3f>("direction", vec3f(0.f, 0.f, 1.f));
  map = (Texture2D *)getParamObject("map", nullptr);

  linear3f frame;
  frame.vx = normalize(-dir);
  frame.vy = normalize(cross(frame.vx, up));
  frame.vz = cross(frame.vx, frame.vy);

  ispc::HDRILight_set(getIE(),
      (const ispc::LinearSpace3f &)frame,
      map ? map->getIE() : nullptr);
}

OSP_REGISTER_LIGHT(HDRILight, hdri);

} // namespace ospray
