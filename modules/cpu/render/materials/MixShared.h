// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/MaterialShared.h"
#include "texture/TextureParamShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Mix
{
  Material super;

  float factor;
  TextureParam factorMap;
  Material *mat1;
  Material *mat2;

#ifdef __cplusplus
  Mix() : factor(.5f), mat1(nullptr), mat2(nullptr)
  {
    super.type = ispc::MATERIAL_TYPE_MIX;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
