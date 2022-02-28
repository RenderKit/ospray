// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/pathtracer/PathTracerDataShared.h"
#include "render/scivis/SciVisDataShared.h"

#ifdef __cplusplus
#include "common/StructShared.h"
namespace ispc {
#endif // __cplusplus

struct Instance;

struct World
{
  Instance **geometriesInst;
  int32 numGeometriesInst;

  Instance **volumesInst;
  int32 numVolumesInst;

  Instance **clippersInst;
  int32 numClippersInst;
  int32 numInvertedClippers;

  RTCScene embreeSceneHandleGeometries;
  RTCScene embreeSceneHandleVolumes;
  RTCScene embreeSceneHandleClippers;

  SciVisData scivisData;
  PathtracerData pathtracerData;

#ifdef __cplusplus
  World()
      : geometriesInst(nullptr),
        numGeometriesInst(0),
        volumesInst(nullptr),
        numVolumesInst(0),
        clippersInst(nullptr),
        numClippersInst(0),
        numInvertedClippers(0),
        embreeSceneHandleGeometries(nullptr),
        embreeSceneHandleVolumes(nullptr),
        embreeSceneHandleClippers(nullptr)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
