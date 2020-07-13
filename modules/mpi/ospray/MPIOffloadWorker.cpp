// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#ifdef _WIN32
#include <WinSock2.h>
#include <process.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <unordered_map>
#include "api/ISPCDevice.h"
#include "common/Library.h"
#include "common/MPIBcastFabric.h"
#include "common/MPICommon.h"
#include "common/OSPWork.h"
#include "common/Profiling.h"
#include "common/SocketBcastFabric.h"
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

  TODO: The worker should now actually just act as an OSPRay user app
  which is using the distributed device, but just receives commands
  from the user's code.

  \internal We assume that mpi::worker and mpi::app have already been set up
*/
void runWorker(bool useMPIFabric)
{
  // Keep a reference to the offload device we came from to keep it alive
  auto offloadDevice = ospray::api::Device::current;

  OSPDevice distribDevice = ospNewDevice("mpiDistributed");
  ospDeviceSetParam(
      distribDevice, "worldCommunicator", OSP_VOID_PTR, &worker.comm);
  ospDeviceCommit(distribDevice);
  ospSetCurrentDevice(distribDevice);

  char hostname[HOST_NAME_MAX] = {0};
  gethostname(hostname, HOST_NAME_MAX);
  postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
      << "#w: running MPI worker process " << workerRank() << "/"
      << workerSize() << " on pid " << getpid() << "@" << hostname;

#ifdef ENABLE_PROFILING
  const std::string worker_log_file =
      std::string(hostname) + "_" + std::to_string(workerRank()) + ".txt";
  std::ofstream fout(worker_log_file.c_str());
  fout << "Worker rank " << workerRank() << " on '" << hostname << "'\n"
       << "/proc/self/status:\n"
       << getProcStatus() << "\n=====\n";
#endif

  std::unique_ptr<networking::Fabric> fabric;
  if (useMPIFabric)
    fabric = make_unique<MPIFabric>(mpicommon::world, 0);
  else
    fabric = make_unique<SocketReaderFabric>(mpicommon::world, 0);

  work::OSPState ospState;

  uint64_t commandSize = 0;
  utility::ArrayView<uint8_t> cmdView(
      reinterpret_cast<uint8_t *>(&commandSize), sizeof(uint64_t));

  std::shared_ptr<utility::OwnedArray<uint8_t>> recvBuffer =
      std::make_shared<utility::OwnedArray<uint8_t>>();

#ifdef ENABLE_PROFILING
  ProfilingPoint workerStart;
#endif
  while (1) {
    using namespace std::chrono;
    auto start = steady_clock::now();
    fabric->recvBcast(cmdView);

    recvBuffer->resize(commandSize, 0);
    fabric->recvBcast(*recvBuffer);

    size_t numCommands = 0;
    // Read each command in the buffer and execute it
    networking::BufferReader reader(recvBuffer);
    while (!reader.end()) {
      ++numCommands;
      work::TAG workTag = work::NONE;
      reader >> workTag;

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#osp.mpi.worker: processing work " << workTag << ": "
          << work::tagName(workTag);

      // We're exiting so sync out our debug log info
#ifdef ENABLE_PROFILING
      if (workTag == work::FINALIZE) {
        ProfilingPoint workerEnd;
        fout << "Worker exiting, /proc/self/status:\n"
             << getProcStatus() << "\n======\n"
             << "Avg. CPU % during run: "
             << cpuUtilization(workerStart, workerEnd) << "%\n"
             << std::flush;
        fout.close();
      }
#endif

      dispatchWork(workTag, ospState, reader, *fabric);

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#osp.mpi.worker: completed work " << workTag << ": "
          << work::tagName(workTag);
    }
    auto end = steady_clock::now();
    std::cout << "Buffer had " << numCommands << " commands, processed in "
              << duration_cast<milliseconds>(end - start).count() << "ms\n";
  }
}
} // namespace mpi
} // namespace ospray
