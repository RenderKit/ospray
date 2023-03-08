// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// We don't want an instance of ObjectFactory static table in this library so
// we set OBJECTFACTORY_IMPORT at the CMake build flag level for the module
#include "common/ObjectFactory.h"

#include "MPIOffloadDevice.h"
#include "common/OSPCommon.h"

extern "C" OSPError OSPRAY_DLLEXPORT ospray_module_init_mpi_offload(
    int16_t versionMajor, int16_t versionMinor, int16_t /*versionPatch*/)
{
  using namespace ospray;

  auto status = moduleVersionCheck(versionMajor, versionMinor);

  if (status == OSP_NO_ERROR) {
    api::Device::registerType<mpi::MPIOffloadDevice>("mpiOffload");
  }

  return status;
}

