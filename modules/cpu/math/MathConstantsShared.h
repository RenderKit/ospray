// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct MathConstants
{
  unsigned int *haltonPerm3;
  unsigned int *haltonPerm5;
  unsigned int *haltonPerm7;

  unsigned int *sobolMatrices;

#ifdef __cplusplus
  MathConstants()
      : haltonPerm3(nullptr),
        haltonPerm5(nullptr),
        haltonPerm7(nullptr),
        sobolMatrices(nullptr)
  {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
