// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/MaterialShared.h"
#include "texture/TextureParamShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Alloy
{
  Material super;

  vec3f color; // reflectivity at normal incidence (0 deg)
  TextureParam colorMap;
  vec3f edgeColor; // reflectivity at grazing angle (90 deg)
  TextureParam edgeColorMap;
  float roughness; // in [0, 1]; 0==ideally smooth (mirror)
  TextureParam roughnessMap;

#ifdef __cplusplus
  Alloy() : color(.9f), edgeColor(1.f), roughness(.1f)
  {
    super.type = ispc::MATERIAL_TYPE_ALLOY;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
