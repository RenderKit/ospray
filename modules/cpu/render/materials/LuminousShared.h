// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/MaterialShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Luminous
{
  Material super;

  float transparency;
  vec3f radiance;
  TextureParam emissionMap;

#ifdef __cplusplus
  Luminous() : super(), transparency(0.f), radiance(1.f)
  {
    super.type = ispc::MATERIAL_TYPE_LUMINOUS;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
