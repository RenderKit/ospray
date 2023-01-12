// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/WorldShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct DistributedWorld
{
  World super;

  box3f *localRegions;
  int numLocalRegions;
  int numRegions;
  RTCScene regionScene;

#ifdef __cplusplus
  DistributedWorld()
      : localRegions(nullptr),
        numLocalRegions(0),
        numRegions(0),
        regionScene(nullptr)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
