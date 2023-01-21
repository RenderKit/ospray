// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture.h"

namespace ospray {

// Texture definitions ////////////////////////////////////////////////////////

Texture::Texture(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext(), device)
{
  managedObjectType = OSP_TEXTURE;
}

std::string Texture::toString() const
{
  return "ospray::Texture";
}

OSPTYPEFOR_DEFINITION(Texture *);

} // namespace ospray
