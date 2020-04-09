// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture.h"
#include "common/Util.h"

namespace ospray {

static FactoryMap<Texture> g_texMap;

// Texture definitions ////////////////////////////////////////////////////////

Texture::Texture()
{
  managedObjectType = OSP_TEXTURE;
}

std::string Texture::toString() const
{
  return "ospray::Texture";
}

Texture *Texture::createInstance(const char *type)
{
  return createInstanceHelper(type, g_texMap[type]);
}

void Texture::registerType(const char *type, FactoryFcn<Texture> f)
{
  g_texMap[type] = f;
}

OSPTYPEFOR_DEFINITION(Texture *);

} // namespace ospray
