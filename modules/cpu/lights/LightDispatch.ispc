// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/DifferentialGeometry.ih"
#include "common/FeatureFlagsEnum.h"
#include "lights/LightDispatch.ih"
#include "lights/LightShared.h"

#include "lights/AmbientLight.ih"
#include "lights/CylinderLight.ih"
#include "lights/DirectionalLight.ih"
#include "lights/HDRILight.ih"
#include "lights/PointLight.ih"
#include "lights/QuadLight.ih"
#include "lights/SpotLight.ih"
#include "render/pathtracer/GeometryLight.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL Light_SampleRes Light_dispatch_sample(const Light *uniform self,
    const DifferentialGeometry &dg,
    const vec2f &s,
    const float time,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);

#ifndef OSPRAY_TARGET_SYCL
  return self->sample(self, dg, s, time, ffh);
#else
  if ((self->type == LIGHT_TYPE_AMBIENT) && (ffo & FFO_LIGHT_AMBIENT)) {
    return AmbientLight_sample(self, dg, s, time, ffh);
  } else if ((self->type == LIGHT_TYPE_DIRECTIONAL)
      && (ffo & FFO_LIGHT_DIRECTIONAL)) {
    return DirectionalLight_sample(self, dg, s, time, ffh);
  } else if ((self->type == LIGHT_TYPE_POINT) && (ffo & FFO_LIGHT_POINT)) {
    return PointLight_sample(self, dg, s, time, ffh);
  } else if ((self->type == LIGHT_TYPE_SPOT) && (ffo & FFO_LIGHT_SPOT)) {
    return SpotLight_sample(self, dg, s, time, ffh);
  } else if ((self->type == LIGHT_TYPE_HDRI) && (ffo & FFO_LIGHT_HDRI)) {
    return HDRILight_sample(self, dg, s, time, ffh);
  } else if ((self->type == LIGHT_TYPE_QUAD) && (ffo & FFO_LIGHT_QUAD)) {
    return QuadLight_sample(self, dg, s, time, ffh);
  } else if ((self->type == LIGHT_TYPE_CYLINDER)
      && (ffo & FFO_LIGHT_CYLINDER)) {
    return CylinderLight_sample(self, dg, s, time, ffh);
  } else if ((self->type == LIGHT_TYPE_GEOMETRY)
      && (ffo & FFO_LIGHT_GEOMETRY)) {
    return GeometryLight_sample(self, dg, s, time, ffh);
  } else {
    Light_SampleRes res;
    res.weight = make_vec3f(0.f);
    res.dir = make_vec3f(0.f);
    res.dist = inf;
    res.pdf = 0.f;
    return res;
  }
#endif
}

SYCL_EXTERNAL Light_EvalRes Light_dispatch_eval(const Light *uniform self,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist,
    const float time,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);

#ifndef OSPRAY_TARGET_SYCL
  return self->eval(self, dg, dir, minDist, maxDist, time);
#else
  if ((self->type == LIGHT_TYPE_AMBIENT) && (ffo & FFO_LIGHT_AMBIENT)) {
    return AmbientLight_eval(self, dg, dir, minDist, maxDist, time);
  } else if ((self->type == LIGHT_TYPE_DIRECTIONAL)
      && (ffo & FFO_LIGHT_DIRECTIONAL)) {
    return DirectionalLight_eval(self, dg, dir, minDist, maxDist, time);
  } else if ((self->type == LIGHT_TYPE_POINT) && (ffo & FFO_LIGHT_POINT)) {
    return PointLight_eval(self, dg, dir, minDist, maxDist, time);
  } else if ((self->type == LIGHT_TYPE_SPOT) && (ffo & FFO_LIGHT_SPOT)) {
    return SpotLight_eval(self, dg, dir, minDist, maxDist, time);
  } else if ((self->type == LIGHT_TYPE_HDRI) && (ffo & FFO_LIGHT_HDRI)) {
    return HDRILight_eval(self, dg, dir, minDist, maxDist, time);
  } else if ((self->type == LIGHT_TYPE_QUAD) && (ffo & FFO_LIGHT_QUAD)) {
    return QuadLight_eval(self, dg, dir, minDist, maxDist, time);
  } else if ((self->type == LIGHT_TYPE_CYLINDER)
      && (ffo & FFO_LIGHT_CYLINDER)) {
    return CylinderLight_eval(self, dg, dir, minDist, maxDist, time);
  } else {
    Light_EvalRes res;
    res.radiance = make_vec3f(0.f);
    res.pdf = 0.f;
    return res;
  }
#endif
}

OSPRAY_END_ISPC_NAMESPACE
