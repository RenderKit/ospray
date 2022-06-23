// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "MediumShared.h"
#include "render/MaterialShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Glass
{
  Material super;

  Medium mediumInside;
  Medium mediumOutside;
};

#ifdef __cplusplus
} // namespace ispc
#endif // __cplusplus
