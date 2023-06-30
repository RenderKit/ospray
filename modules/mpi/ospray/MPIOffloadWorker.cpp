// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ospray/ospray.h"

#include "rkcommon/platform.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <process.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <unordered_map>
#include "MPIOffloadDevice.h"
#include "common/Library.h"
#include "common/MPIBcastFabric.h"
#include "common/MPICommon.h"
#include "common/OSPWork.h"
// Only temporary location for these files anyways
#include "rkcommon/tracing/Tracing.h"
#include "rkcommon/utility/getEnvVar.h"

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 4096
#endif

namespace ospray {
namespace mpi {

using namespace mpicommon;

/*! it's up to the proper init
  routine to decide which processes call this function and which
  ones don't. This function will not return.

  This function takes the offload device that spawned the workers
  so that it can be cleaned up at exit properly, as the workers are
  started on commit and do not return back to the application code.
  ospShutdown is not called as it would unload the shared library
  with the code and object being used here, and when created explicitly
  the offload device would not be set as the current device or visible
  to be cleaned up.

  \internal We assume that mpi::worker and mpi::app have already been set up
*/
void runWorker(bool useMPIFabric, MPIOffloadDevice *offloadDevice)
{
  // Nested scope to ensure all OSPRay objects/etc. are cleaned up
  // before we exit
  {
    // We need to remove the offload device so that we call loadLocalModule
    // directly to load the mpi_distributed module, instead of trying to go
    // through the offload device's loadModule implementation which would try to
    // send the command to some "workers".
    ospSetCurrentDevice(nullptr);
    auto OSPRAY_MPI_DISTRIBUTED_GPU =
        utility::getEnvVar<int>("OSPRAY_MPI_DISTRIBUTED_GPU").value_or(0);
    if (OSPRAY_MPI_DISTRIBUTED_GPU) {
      ospLoadModule("mpi_distributed_gpu");
    } else {
      ospLoadModule("mpi_distributed_cpu");
    }

    OSPDevice distribDevice = ospNewDevice("mpiDistributed");
    ospDeviceSetParam(
        distribDevice, "worldCommunicator", OSP_VOID_PTR, &worker.comm);
    ospDeviceCommit(distribDevice);
    ospSetCurrentDevice(distribDevice);
    // Release our local reference to the device
    ospDeviceRelease(distribDevice);

    char hostname[HOST_NAME_MAX] = {0};
    gethostname(hostname, HOST_NAME_MAX);

    rkTraceBeginEvent("runMPIOffloadWorker");

#ifdef ENABLE_PROFILING
    // TODO: Add this data to the JSON trace file? Or separate file
    /*
    const std::string worker_log_file =
        std::string(hostname) + "_" + std::to_string(workerRank()) + ".txt";
    std::ofstream fout(worker_log_file.c_str());
    fout << "Worker rank " << workerRank() << " on '" << hostname << "'\n"
         << "/proc/self/status:\n"
         << getProcStatus() << "\n=====\n";
         */
#endif

    std::unique_ptr<networking::Fabric> fabric;
    if (useMPIFabric)
      fabric = make_unique<MPIFabric>(mpicommon::world, 0);
    else
      throw std::runtime_error("Invalid non-MPI connection mode");

    work::OSPState ospState;

    uint64_t commandSize = 0;
    utility::ArrayView<uint8_t> cmdView(
        reinterpret_cast<uint8_t *>(&commandSize), sizeof(uint64_t));

    std::shared_ptr<utility::OwnedArray<uint8_t>> recvBuffer =
        std::make_shared<utility::OwnedArray<uint8_t>>();

    rkTraceSetThreadName("main");

    bool exitWorker = false;
    while (!exitWorker) {
      fabric->recvBcast(cmdView);

      recvBuffer->resize(commandSize, 0);
      fabric->recvBcast(*recvBuffer);

      // Read each command in the buffer and execute it
      networking::BufferReader reader(recvBuffer);
      while (!reader.end()) {
        work::TAG workTag = work::NONE;
        reader >> workTag;

        rkTraceSetMarker(work::tagName(workTag));
        if (workTag == work::FINALIZE) {
          exitWorker = true;
        } else {
          dispatchWork(workTag, ospState, reader, *fabric);
        }
        rkTraceRecordMemUse();

        // Snapshot after each renderframe for bentley, not sure on what input
        // to cleanly exit the server. Need to ask Alok
        if (workTag == work::MAP_FRAMEBUFFER) {
          const std::string workerTraceFile = std::string(hostname) + "_"
              + std::to_string(mpicommon::workerRank()) + ".json";
          rkTraceSaveLog(workerTraceFile.c_str(), workerTraceFile.c_str());
        }
      }
    }
    rkTraceEndEvent();

    // The API no longer knows about the offload device and we won't return back
    // to the app to release it, so we need to free it now
    offloadDevice->refDec();
    // Explicitly release the distributed device by unsetting the current
    // device, to avoid it attempting to be freed again on exit
    ospray::api::Device::current = nullptr;

    /*
    fout << "Worker exiting, /proc/self/status:\n"
         << getProcStatus() << "\n======\n"
         << "Avg. CPU % during run: "
         << cpuUtilization(workerStart, workerEnd) << "%\n"
         << std::flush;
    fout.close();
    */
  }

  MPI_CALL(Comm_free(&worker.comm));
  // The offload device initialized MPI, so the distributed device will see
  // the "app" as having already initialized MPI and assume it should not call
  // finalize. So the worker loop must call MPI finalize here as if it was a
  // distributed app.
  MPI_CALL(Finalize());
  std::exit(0);
}
} // namespace mpi
} // namespace ospray
