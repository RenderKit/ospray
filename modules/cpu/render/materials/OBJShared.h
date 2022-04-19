// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/MaterialShared.h"
#include "texture/TextureParamShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct OBJ
{
  Material super;

  float d; // cut-out opacity
  TextureParam dMap;
  vec3f Kd;
  TextureParam KdMap;
  vec3f Ks;
  TextureParam KsMap;
  float Ns;
  TextureParam NsMap;
  vec3f Tf; // transmission filter
  TextureParam bumpMap;
  linear2f bumpRot; // just the inverse of rotational/mirror part (must be
                    // orthonormal) of tc xfrom

#ifdef __cplusplus
  OBJ() : d(1.f), Kd(.8f), Ks(0.f), Ns(2.f), Tf(0.f), bumpRot(one)
  {
    super.type = MATERIAL_TYPE_OBJ;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
