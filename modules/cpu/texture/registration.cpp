// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture2D.h"
#ifdef OSPRAY_ENABLE_VOLUMES
#include "TextureVolume.h"
#endif

namespace ospray {

void registerAllTextures()
{
  Texture::registerType<Texture2D>("texture2d");
#ifdef OSPRAY_ENABLE_VOLUMES
  Texture::registerType<TextureVolume>("volume");
#endif
}

} // namespace ospray
