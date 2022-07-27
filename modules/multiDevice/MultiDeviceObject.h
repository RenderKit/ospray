// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/Data.h"

namespace ospray {
namespace api {

// An object in the multidevice is just a container to hold
// the handles to the actual objects of each subdevice
struct MultiDeviceObject : public memory::RefCount
{
  std::vector<OSPObject> objects;

  // sharedDataDirtyReference is held temporarily to ensure consistency
  Data *sharedDataDirtyReference = nullptr;
  virtual ~MultiDeviceObject() override
  {
    delete sharedDataDirtyReference;
  }
};

} // namespace api
} // namespace ospray
