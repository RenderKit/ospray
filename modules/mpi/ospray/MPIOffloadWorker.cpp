// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#ifdef _WIN32
#include <process.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <algorithm>
#include <unordered_map>
#include "api/ISPCDevice.h"
#include "camera/Camera.h"
#include "common/Data.h"
#include "common/Library.h"
#include "common/World.h"
#include "fb/LocalFB.h"
#include "geometry/TriangleMesh.h"
#include "lights/Light.h"
#include "common/MPIBcastFabric.h"
#include "common/SocketBcastFabric.h"
#include "common/MPICommon.h"
#include "ospcommon/utility/getEnvVar.h"
#include "render/Renderer.h"
#include "texture/Texture2D.h"
#include "transferFunction/TransferFunction.h"
#include "volume/Volume.h"
#include "fb/DistributedFrameBuffer.h"
#include "common/OSPWork.h"

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

      OSPDevice distribDevice = ospNewDevice("mpi_distributed");
      ospDeviceSetVoidPtr(distribDevice, "worldCommunicator", &worker.comm);
      ospDeviceCommit(distribDevice);
      ospSetCurrentDevice(distribDevice);

      char hostname[HOST_NAME_MAX];
      gethostname(hostname, HOST_NAME_MAX);
      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#w: running MPI worker process " << workerRank() << "/"
          << workerSize() << " on pid " << getpid() << "@" << hostname;

      std::unique_ptr<networking::Fabric> fabric;
      if (useMPIFabric)
        fabric = make_unique<MPIFabric>(mpicommon::world, 0);
      else
        fabric = make_unique<SocketReaderFabric>(mpicommon::world, 0);

      work::OSPState ospState;

      uint64_t commandSize = 0;
      utility::ArrayView<uint8_t> cmdView(reinterpret_cast<uint8_t*>(&commandSize),
                                          sizeof(uint64_t));

      std::shared_ptr<utility::OwnedArray<uint8_t>> recvBuffer
        = std::make_shared<utility::OwnedArray<uint8_t>>();

      while (1) {
        fabric->recvBcast(cmdView);

        recvBuffer->resize(commandSize, 0);
        fabric->recvBcast(*recvBuffer);

        networking::BufferReader reader(recvBuffer);
        work::TAG workTag = work::NONE;
        reader >> workTag;

        postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#osp.mpi.worker: processing work " << workTag
          << ": " << work::tagName(workTag);

        dispatchWork(workTag, ospState, reader, *fabric);

        postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#osp.mpi.worker: completed work " << workTag
          << ": " << work::tagName(workTag);
      }
    }
  }
}

