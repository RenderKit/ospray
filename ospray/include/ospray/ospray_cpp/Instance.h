// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Group.h"

namespace ospray {
namespace cpp {

class Instance : public ManagedObject<OSPInstance, OSP_INSTANCE>
{
 public:
  Instance(Group &group);
  Instance(OSPGroup group);
  Instance(OSPInstance existing = nullptr);
};

static_assert(sizeof(Instance) == sizeof(OSPInstance),
    "cpp::Instance can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline Instance::Instance(Group &group) : Instance(group.handle()) {}

inline Instance::Instance(OSPGroup group)
{
  ospObject = ospNewInstance(group);
}

inline Instance::Instance(OSPInstance existing)
    : ManagedObject<OSPInstance, OSP_INSTANCE>(existing)
{}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::Instance, OSP_INSTANCE);

} // namespace ospray
