// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ManagedObject.h"

namespace ospray {
namespace cpp {

class Light : public ManagedObject<OSPLight, OSP_LIGHT>
{
 public:
  Light(const std::string &light_type);
  Light(const Light &copy);
  Light(OSPLight existing = nullptr);
};

static_assert(
    sizeof(Light) == sizeof(OSPLight), "cpp::Light can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline Light::Light(const std::string &light_type)
{
  ospObject = ospNewLight(light_type.c_str());
}

inline Light::Light(const Light &copy)
    : ManagedObject<OSPLight, OSP_LIGHT>(copy.handle())
{
  ospRetain(copy.handle());
}

inline Light::Light(OSPLight existing)
    : ManagedObject<OSPLight, OSP_LIGHT>(existing)
{}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::Light, OSP_LIGHT);

} // namespace ospray
