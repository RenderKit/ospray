// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture.h"
#include "common/Util.h"

namespace ospray {

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
  return createInstanceHelper<Texture, OSP_TEXTURE>(type);
}

OSPTYPEFOR_DEFINITION(Texture *);

} // namespace ospray
