// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray stuff
#include "./Group.h"
// embree
#include "embree3/rtcore.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Instance : public ManagedObject
{
  Instance(Group *group);
  ~Instance() override = default;

  std::string toString() const override;

  void commit() override;

  box3f getBounds() const override;

  affine3f xfm();

  // Data //

  affine3f instanceXfm;
  affine3f rcpXfm;

  Ref<Group> group;
};

OSPTYPEFOR_SPECIALIZATION(Instance *, OSP_INSTANCE);

// Inlined definitions //////////////////////////////////////////////////////

inline affine3f Instance::xfm()
{
  return instanceXfm;
}

} // namespace ospray
