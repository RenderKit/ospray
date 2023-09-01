// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#ifdef OSPRAY_ENABLE_VOLUMES

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

  const Volume *volume;
  const TransferFunction *transferFunction;

#ifdef __cplusplus
  TextureVolume() : super(true), volume(nullptr), transferFunction(nullptr)
  {
    super.type = TEXTURE_TYPE_VOLUME;
  }
};
} // namespace ispc
#else
};
#endif // __cplusplus
#endif
