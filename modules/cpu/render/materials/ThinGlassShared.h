// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/MaterialShared.h"
#include "texture/TextureParamShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct ThinGlass
{
  Material super;

  float eta; // reciprocal refraction index of internal medium
             // assumed to be <=1
  vec3f attenuation; // negative Napierian attenuation coefficient,
                     // i.e. wrt. the natural base e
  float attenuationScale; // factor to scale attenuation from texture due to
                          // thickness and attenuationDistance
  TextureParam attenuationColorMap;

#ifdef __cplusplus
  ThinGlass() : eta(1.5f), attenuation(0.f), attenuationScale(1.f)
  {
    super.type = ispc::MATERIAL_TYPE_THINGLASS;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
