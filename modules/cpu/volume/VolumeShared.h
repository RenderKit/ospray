// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include "common/StructShared.h"
namespace ispc {
#endif // __cplusplus

// Variables and methods common to all subtypes of the Volume
// class, an abstraction for the concrete object which performs the
// volume sampling (this struct must be the first field of a struct
// representing a "derived" class to allow casting to that class).
struct Volume
{
  VKLVolume vklVolume;
  VKLSampler vklSampler;

  // Bounding box for the volume in world coordinates.
  // This is an internal derived parameter and not meant to be
  // redefined externally.
  box3f boundingBox;

#ifdef __cplusplus
  Volume() : vklVolume(nullptr), vklSampler(nullptr), boundingBox(0.f, 0.f) {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
