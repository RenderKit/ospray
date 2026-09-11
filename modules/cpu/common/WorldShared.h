// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "render/pathtracer/PathTracerDataShared.h"
#include "render/scivis/SciVisDataShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Instance;

struct World
{
  Instance **instances;
  int32 numInvertedClippers;

  RTCTraversable embreeTraversableHandleGeometries;
#ifdef OSPRAY_ENABLE_VOLUMES
  RTCTraversable embreeTraversableHandleVolumes;
#endif
#ifndef OSPRAY_TARGET_SYCL
  RTCTraversable embreeTraversableHandleClippers;
#endif

  SciVisData *scivisData;
  PathTracerData *pathtracerData;

#ifdef __cplusplus
  World()
      : instances(nullptr),
        numInvertedClippers(0),
        embreeTraversableHandleGeometries(nullptr),
#ifdef OSPRAY_ENABLE_VOLUMES
        embreeTraversableHandleVolumes(nullptr),
#endif
#ifndef OSPRAY_TARGET_SYCL
        embreeTraversableHandleClippers(nullptr),
#endif
        scivisData(nullptr),
        pathtracerData(nullptr)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
