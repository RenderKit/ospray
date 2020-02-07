// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ManagedObject.h"

namespace ospray {
namespace cpp {

class Geometry : public ManagedObject<OSPGeometry, OSP_GEOMETRY>
{
 public:
  Geometry(const std::string &type);
  Geometry(const Geometry &copy);
  Geometry(OSPGeometry existing = nullptr);
};

static_assert(sizeof(Geometry) == sizeof(OSPGeometry),
    "cpp::Geometry can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline Geometry::Geometry(const std::string &type)
{
  ospObject = ospNewGeometry(type.c_str());
}

inline Geometry::Geometry(const Geometry &copy)
    : ManagedObject<OSPGeometry, OSP_GEOMETRY>(copy.handle())
{
  ospRetain(copy.handle());
}

inline Geometry::Geometry(OSPGeometry existing)
    : ManagedObject<OSPGeometry, OSP_GEOMETRY>(existing)
{}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::Geometry, OSP_GEOMETRY);

} // namespace ospray
