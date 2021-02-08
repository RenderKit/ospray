// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ManagedObject.h"

namespace ospray {
namespace cpp {

class Texture : public ManagedObject<OSPTexture, OSP_TEXTURE>
{
 public:
  Texture(const std::string &type);
  Texture(OSPTexture existing = nullptr);
};

static_assert(sizeof(Texture) == sizeof(OSPTexture),
    "cpp::Texture can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline Texture::Texture(const std::string &type)
{
  ospObject = ospNewTexture(type.c_str());
}

inline Texture::Texture(OSPTexture existing)
    : ManagedObject<OSPTexture, OSP_TEXTURE>(existing)
{}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::Texture, OSP_TEXTURE);

} // namespace ospray
