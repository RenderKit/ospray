// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/MaterialShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Velvet
{
  Material super;

  // Diffuse reflectance of the surface. The range is from 0
  // (black) to 1 (white).
  vec3f reflectance;

  // Amount of back scattering. The range is from 0 (no back
  // scattering) to inf (maximum back scattering).
  float backScattering;

  // Color of horizon scattering.
  vec3f horizonScatteringColor;

  // Fall-off of horizon scattering.
  float horizonScatteringFallOff;

#ifdef __cplusplus
  Velvet()
      : reflectance(.4f, 0.f, 0.f),
        backScattering(.5f),
        horizonScatteringColor(.75f, .1f, .1f),
        horizonScatteringFallOff(10)
  {
    super.type = ispc::MATERIAL_TYPE_VELVET;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
