// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Instance.h"
#include "ManagedObject.h"

namespace ospray {
namespace cpp {

class World : public ManagedObject<OSPWorld, OSP_WORLD>
{
 public:
  World();
  World(OSPWorld existing);
};

static_assert(
    sizeof(World) == sizeof(OSPWorld), "cpp::World can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline World::World()
{
  ospObject = ospNewWorld();
}

inline World::World(OSPWorld existing)
    : ManagedObject<OSPWorld, OSP_WORLD>(existing)
{}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::World, OSP_WORLD);

} // namespace ospray
