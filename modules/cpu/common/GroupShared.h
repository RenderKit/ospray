// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct GeometricModel;
#ifdef OSPRAY_ENABLE_VOLUMES
struct VolumetricModel;
#endif

struct Group
{
  GeometricModel **geometricModels;
  int32 numGeometricModels;

#ifdef OSPRAY_ENABLE_VOLUMES
  VolumetricModel **volumetricModels;
  int32 numVolumetricModels;
#endif

  GeometricModel **clipModels;
  int32 numClipModels;

#ifdef __cplusplus
  Group()
      : geometricModels(nullptr),
        numGeometricModels(0),
#ifdef OSPRAY_ENABLE_VOLUMES
        volumetricModels(nullptr),
        numVolumetricModels(0),
#endif
        clipModels(nullptr),
        numClipModels(0)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
