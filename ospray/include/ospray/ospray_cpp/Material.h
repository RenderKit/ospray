// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ManagedObject.h"

namespace ospray {
namespace cpp {

class Material : public ManagedObject<OSPMaterial, OSP_MATERIAL>
{
 public:
  Material(const std::string &renderer_type, const std::string &mat_type);
  Material(const Material &copy);
  Material(OSPMaterial existing = nullptr);
};

static_assert(sizeof(Material) == sizeof(OSPMaterial),
    "cpp::Material can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline Material::Material(
    const std::string &renderer_type, const std::string &mat_type)
{
  ospObject = ospNewMaterial(renderer_type.c_str(), mat_type.c_str());
}

inline Material::Material(const Material &copy)
    : ManagedObject<OSPMaterial, OSP_MATERIAL>(copy.handle())
{
  ospRetain(copy.handle());
}

inline Material::Material(OSPMaterial existing)
    : ManagedObject<OSPMaterial, OSP_MATERIAL>(existing)
{}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::Material, OSP_MATERIAL);

} // namespace ospray
