// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include "common/StructShared.h"
namespace ispc {
#endif // __cplusplus

struct GeometricModel;
struct VolumetricModel;

struct Group
{
  GeometricModel **geometricModels;
  int32 numGeometricModels;

  VolumetricModel **volumetricModels;
  int32 numVolumetricModels;

  GeometricModel **clipModels;
  int32 numClipModels;

#ifdef __cplusplus
  Group()
      : geometricModels(nullptr),
        numGeometricModels(0),
        volumetricModels(nullptr),
        numVolumetricModels(0),
        clipModels(nullptr),
        numClipModels(0)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
