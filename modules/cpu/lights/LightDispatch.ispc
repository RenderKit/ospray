// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/DifferentialGeometry.ih"
#include "lights/LightDispatch.ih"
#include "lights/LightShared.h"

#include "lights/QuadLight.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL __noinline Light_SampleRes Light_dispatch_sample(
    const Light *uniform self,
    const DifferentialGeometry &dg,
    const vec2f &s,
    const float time)
{
  switch (self->type) {
  case LIGHT_TYPE_QUAD:
    return QuadLight_sample(self, dg, s, time);
  default:
#ifndef OSPRAY_TARGET_SYCL
    return self->sample(self, dg, s, time);
#else
    break;
#endif
  }
#ifdef OSPRAY_TARGET_SYCL
  Light_SampleRes res;
  res.weight = make_vec3f(0.f);
  res.dir = make_vec3f(0.f);
  res.dist = inf;
  res.pdf = 0.f;
  return res;
#endif
}

SYCL_EXTERNAL __noinline Light_EvalRes Light_dispatch_eval(
    const Light *uniform self,
    const DifferentialGeometry &dg,
    const vec3f &dir,
    const float minDist,
    const float maxDist,
    const float time)
{
  switch (self->type) {
  case LIGHT_TYPE_QUAD:
    return QuadLight_eval(self, dg, dir, minDist, maxDist, time);
  default:
#ifndef OSPRAY_TARGET_SYCL
    return self->eval(self, dg, dir, minDist, maxDist, time);
#else
    break;
#endif
  }
#ifdef OSPRAY_TARGET_SYCL
  Light_EvalRes res;
  res.radiance = make_vec3f(0.f);
  res.pdf = 0.f;
  return res;
#endif
}

OSPRAY_END_ISPC_NAMESPACE
