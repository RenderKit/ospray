// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/pathtracer/PathTracerDataShared.h"
#include "render/scivis/SciVisDataShared.h"

#ifdef __cplusplus
#include "embree4/rtcore.h"
#else
#include "embree4/rtcore.isph"
#endif

#ifdef __cplusplus
#include "common/StructShared.h"
namespace ispc {
#endif // __cplusplus

struct Instance;

struct World
{
  Instance **instances;
  int32 numInvertedClippers;

  RTCScene embreeSceneHandleGeometries;
#ifdef OSPRAY_ENABLE_VOLUMES
  RTCScene embreeSceneHandleVolumes;
#endif
  RTCScene embreeSceneHandleClippers;

  SciVisData *scivisData;
  PathTracerData *pathtracerData;

#ifdef __cplusplus
  World()
      : instances(nullptr),
        numInvertedClippers(0),
        embreeSceneHandleGeometries(nullptr),
#ifdef OSPRAY_ENABLE_VOLUMES
        embreeSceneHandleVolumes(nullptr),
#endif
        embreeSceneHandleClippers(nullptr),
        scivisData(nullptr),
        pathtracerData(nullptr)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
