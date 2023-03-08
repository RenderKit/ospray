// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "TransferFunctionShared.h"

#define PRECOMPUTED_OPACITY_SUBRANGE_COUNT 32

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct LinearTransferFunction
{
  TransferFunction super;

  Data1D color;
  Data1D opacity;

  // precomputed maximum opacity values per range
  float maxOpacityInRange[PRECOMPUTED_OPACITY_SUBRANGE_COUNT]
                         [PRECOMPUTED_OPACITY_SUBRANGE_COUNT];
#ifdef __cplusplus
  LinearTransferFunction()
  {
    super.type = TRANSFER_FUNCTION_TYPE_LINEAR;
  }
#endif
};
#ifdef __cplusplus
} // namespace ispc
#endif // __cplusplus
