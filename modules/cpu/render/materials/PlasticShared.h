// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/MaterialShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Plastic
{
  Material super;

  vec3f pigmentColor;
  float eta;
  float roughness;

#ifdef __cplusplus
  Plastic() : pigmentColor(1.f), eta(1.4f), roughness(0.01f)
  {
    super.type = ispc::MATERIAL_TYPE_PLASTIC;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
