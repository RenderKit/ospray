// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
#include "common/StructShared.h"
namespace ispc {
typedef const unsigned int *ConstantsDataUint;
#else
typedef const uniform unsigned int *uniform ConstantsDataUint;
#endif // __cplusplus

struct MathConstants
{
  ConstantsDataUint halton_perm3;
  ConstantsDataUint halton_perm5;
  ConstantsDataUint halton_perm7;

  ConstantsDataUint sobol_matrices;

#ifdef __cplusplus
  MathConstants()
      : halton_perm3(nullptr),
        halton_perm5(nullptr),
        halton_perm7(nullptr),
        sobol_matrices(nullptr)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
