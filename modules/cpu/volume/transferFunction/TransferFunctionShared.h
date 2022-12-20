// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

// We only have one transfer function type, but we still have the type enum and
// "dispatch" function so that we can easily extend it with more in the future
enum TransferFunctionType
{
  TRANSFER_FUNCTION_TYPE_LINEAR = 0,
  TRANSFER_FUNCTION_TYPE_UNKNOWN = 1,
};

struct TransferFunction
{
  TransferFunctionType type;
  range1f valueRange;

#ifdef __cplusplus
  TransferFunction()
      : type(TRANSFER_FUNCTION_TYPE_UNKNOWN), valueRange(0.f, 1.f)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
