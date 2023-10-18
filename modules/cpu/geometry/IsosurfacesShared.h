// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#ifdef OSPRAY_ENABLE_VOLUMES

#pragma once

#include "GeometryShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Isosurfaces
{
  Geometry super; // inherited geometry fields
  float *isovalues;
  Volume *volume;
  VKLHitIteratorContext vklHitContext;

#ifdef __cplusplus
  Isosurfaces()
      : isovalues(nullptr),
        volume(nullptr),
        vklHitContext(VKLHitIteratorContext())
  {
    super.type = ispc::GEOMETRY_TYPE_ISOSURFACES;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
#endif
