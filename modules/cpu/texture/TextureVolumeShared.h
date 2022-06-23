// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "TextureShared.h"

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Volume;
struct TransferFunction;

struct TextureVolume
{
  Texture super;

  const Volume *volume; // instanced Volume

  // Color and opacity transfer function.
  const TransferFunction *transferFunction;

#ifdef __cplusplus
  TextureVolume() : super(true), volume(nullptr), transferFunction(nullptr) {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
