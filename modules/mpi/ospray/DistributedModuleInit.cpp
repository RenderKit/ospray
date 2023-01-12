// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// We don't want an instance of ObjectFactory static table in this library so
// we set OBJECTFACTORY_IMPORT at the CMake build flag level for the module
#include "common/ObjectFactory.h"

#include "MPIDistributedDevice.h"
#include "common/OSPCommon.h"
#include "render/distributed/DistributedRaycast.h"

extern "C" OSPError OSPRAY_DLLEXPORT
#ifdef OSPRAY_TARGET_SYCL
ospray_module_init_mpi_distributed_gpu(
#else
ospray_module_init_mpi_distributed_cpu(
#endif
    int16_t versionMajor, int16_t versionMinor, int16_t /*versionPatch*/)
{
  using namespace ospray;

  auto status = moduleVersionCheck(versionMajor, versionMinor);

  if (status == OSP_NO_ERROR) {
    // Run the CPU/GPU module's initialization function to register local types
#ifdef OSPRAY_TARGET_SYCL
    status = ospLoadModule("gpu");
#else
    status = ospLoadModule("cpu");
#endif
  }

  if (status == OSP_NO_ERROR) {
    api::Device::registerType<mpi::MPIDistributedDevice>("mpiDistributed");
    Renderer::registerType<mpi::DistributedRaycastRenderer>("mpiRaycast");
  }

  return status;
}
