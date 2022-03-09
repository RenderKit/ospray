// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MPIDistributedDevice.h"
#include "MPIOffloadDevice.h"
#include "common/OSPCommon.h"
#include "render/distributed/DistributedRaycast.h"

extern "C" OSPError OSPRAY_DLLEXPORT ospray_module_init_mpi(
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
    api::Device::registerType<mpi::MPIDistributedDevice>("mpiDistributed");
    api::Device::registerType<mpi::MPIOffloadDevice>("mpiOffload");
    Renderer::registerType<mpi::DistributedRaycastRenderer>("mpiRaycast");
  }

  return status;
}
