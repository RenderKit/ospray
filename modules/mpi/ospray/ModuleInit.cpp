// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MPIDistributedDevice.h"
#include "MPIOffloadDevice.h"
#include "common/OSPCommon.h"
#include "render/distributed/DistributedRaycast.h"

extern "C" OSPError ospray_module_init_mpi(
    int16_t versionMajor, int16_t versionMinor, int16_t /*versionPatch*/)
{
  using namespace ospray;
  // Run the ISPC module's initialization function as well to register local
  // types
  ospLoadModule("ispc");
  api::Device::registerType<mpi::MPIDistributedDevice>("mpiDistributed");
  api::Device::registerType<mpi::MPIOffloadDevice>("mpiOffload");
  Renderer::registerType<mpi::DistributedRaycastRenderer>("mpiRaycast");
  return moduleVersionCheck(versionMajor, versionMinor);
}
