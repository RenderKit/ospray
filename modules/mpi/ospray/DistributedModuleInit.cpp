// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// TODO: I still get the issue of the 2 object factory declarations if I don't
// set the define as a compile define for the entire library. However this
// doesn't happen if I'm building the unified MPI module. I'm not sure how it
// worked in the unified library, if it did have 2 definitions but happened to
// pull from the one that got populated, or what was going on.
//
// We don't want an instance of ObjectFactory static table in this library so we
// have to include it with import define so the table will be imported from
// 'ospray' library
//#define OBJECTFACTORY_IMPORT
#include "common/ObjectFactory.h"

#include "MPIDistributedDevice.h"
#include "common/OSPCommon.h"
#include "render/distributed/DistributedRaycast.h"

extern "C" OSPError OSPRAY_DLLEXPORT
ospray_module_init_mpi_distributed(
    int16_t versionMajor, int16_t versionMinor, int16_t /*versionPatch*/)
{
  using namespace ospray;

  auto status = moduleVersionCheck(versionMajor, versionMinor);

  if (status == OSP_NO_ERROR) {
    status = ospLoadModule("cpu");
  }

  if (status == OSP_NO_ERROR) {
    api::Device::registerType<mpi::MPIDistributedDevice>("mpiDistributed");
    Renderer::registerType<mpi::DistributedRaycastRenderer>("mpiRaycast");
  }

  return status;
}
