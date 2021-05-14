// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MultiDevice.h"
#include "MultiDeviceLoadBalancer.h"
#include "common/OSPCommon.h"

extern "C" OSPError ospray_module_init_multidevice(
    int16_t versionMajor, int16_t versionMinor, int16_t /*versionPatch*/)
{
  using namespace ospray;

  auto status = moduleVersionCheck(versionMajor, versionMinor);

  if (status == OSP_NO_ERROR) {
    // Run the ISPC module's initialization function as well to register local
    // types
    status = ospLoadModule("ispc");
  }

  if (status == OSP_NO_ERROR) {
    api::Device::registerType<ospray::api::MultiDevice>("multidevice");
    //LoadBalancer::registerType<ospray::MultiDeviceLoadBalancer>("multidevice");
  }

  return status;
}
