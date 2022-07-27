// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>

#include "ManagedObject.h"

namespace ospray {
namespace cpp {

class Volume : public ManagedObject<OSPVolume, OSP_VOLUME>
{
 public:
  Volume(const std::string &type);
  Volume(OSPVolume existing = nullptr);
};

static_assert(sizeof(Volume) == sizeof(OSPVolume),
    "cpp::Volume can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline Volume::Volume(const std::string &type)
{
  ospObject = ospNewVolume(type.c_str());
}

inline Volume::Volume(OSPVolume existing)
    : ManagedObject<OSPVolume, OSP_VOLUME>(existing)
{}
} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::Volume, OSP_VOLUME);

} // namespace ospray
