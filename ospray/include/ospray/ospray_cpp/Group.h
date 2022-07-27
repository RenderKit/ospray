// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ManagedObject.h"

namespace ospray {
namespace cpp {

class Group : public ManagedObject<OSPGroup, OSP_GROUP>
{
 public:
  Group();
  Group(OSPGroup existing);
};

static_assert(
    sizeof(Group) == sizeof(OSPGroup), "cpp::Group can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline Group::Group()
{
  ospObject = ospNewGroup();
}

inline Group::Group(OSPGroup existing)
    : ManagedObject<OSPGroup, OSP_GROUP>(existing)
{}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::Group, OSP_GROUP);

} // namespace ospray
