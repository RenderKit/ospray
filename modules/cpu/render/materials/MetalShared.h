// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/MaterialShared.h"
#include "texture/TextureParamShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Metal
{
  Material super;

  bool spectral;
  spectrum eta; // index of refraction
  spectrum k; // index of refraction, imaginary part
  vec3f etaRGB; // index of refraction
  vec3f kRGB; // index of refraction, imaginary part
  float roughness; // in [0, 1]; 0==ideally smooth (mirror)
  TextureParam roughnessMap;

#ifdef __cplusplus
  Metal()
      : spectral(false),
        eta{},
        k{},
        etaRGB(RGB_AL_ETA),
        kRGB(RGB_AL_K),
        roughness(.1f)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
