// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray
#include "ISPCDevice.h"
#include "common/Managed.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE ISPCDeviceObject : public ManagedObject
{
  ISPCDeviceObject(api::ISPCDevice &device) : ispcDevice(&device) {}

  inline api::ISPCDevice &getISPCDevice() const
  {
    return *ispcDevice;
  }

  template <typename T, int DIM = 1>
  const Ref<const DataT<T, DIM>> getParamDataT(
      const char *name, bool required = false, bool promoteScalar = false);

 private:
  // Ref<api::ISPCDevice> ispcDevice; TODO: const problem
  api::ISPCDevice *ispcDevice{nullptr};
};

} // namespace ospray