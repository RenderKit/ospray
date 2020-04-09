// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture2D.h"
#include "TextureVolume.h"

namespace ospray {

void registerAllTextures()
{
  Texture::registerType<Texture2D>("texture2d");
  Texture::registerType<TextureVolume>("volume");
}

} // namespace ospray
