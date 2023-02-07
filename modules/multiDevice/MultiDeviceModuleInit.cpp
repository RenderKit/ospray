// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// We don't want an instance of ObjectFactory static table in this library
// so we have to include it with import define so the table will be imported
// from 'ospray' library
#include "common/ObjectFactory.h"

#include "MultiDevice.h"
#include "MultiDeviceLoadBalancer.h"
#include "common/OSPCommon.h"

extern "C" OSPError OSPRAY_DLLEXPORT
#ifdef OSPRAY_TARGET_SYCL
ospray_module_init_multidevice_gpu(
#else
ospray_module_init_multidevice_cpu(
#endif
    int16_t versionMajor, int16_t versionMinor, int16_t /*versionPatch*/)
{
  using namespace ospray;

  auto status = moduleVersionCheck(versionMajor, versionMinor);

  if (status == OSP_NO_ERROR) {
    // Run the ISPC module's initialization function as well to register local
    // types
#ifdef OSPRAY_TARGET_SYCL
    status = ospLoadModule("gpu");
#else
    status = ospLoadModule("cpu");
#endif
  }

  if (status == OSP_NO_ERROR) {
    api::Device::registerType<ospray::api::MultiDevice>("multidevice");
  }

  return status;
}
