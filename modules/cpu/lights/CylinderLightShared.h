// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "LightShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

// state that changes under transformations
struct CylinderLightDynamic
{
  vec3f position0;
  vec3f position1;

#ifdef __cplusplus
  CylinderLightDynamic() : position0(0.f), position1(0.f, 0.f, 1.f) {}
#endif
};

struct CylinderLight
{
  Light super;
  vec3f radiance;
  float radius;
  CylinderLightDynamic pre; // un- or pre-transformed state

#ifdef __cplusplus
  CylinderLight() : radiance(1.f), radius(1.f) {}
};
#ifdef OSPRAY_TARGET_SYCL
void CylinderLight_Transform(const void *self, const void *xfm, void *dyn);
void *uniform CylinderLight_sample_addr();
void *uniform CylinderLight_sample_instanced_addr();
void *uniform CylinderLight_eval_addr();
void *uniform CylinderLight_eval_instanced_addr();
#endif
} // namespace ispc
#else
};
#endif // __cplusplus
