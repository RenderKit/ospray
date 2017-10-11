// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "mpiCommon/MPICommon.h"
#include "mpiCommon/MPIBcastFabric.h"
#include "mpi/MPIOffloadDevice.h"
#include "common/Model.h"
#include "common/Data.h"
#include "common/Library.h"
#include "common/Model.h"
#include "geometry/TriangleMesh.h"
#include "render/Renderer.h"
#include "camera/Camera.h"
#include "volume/Volume.h"
#include "lights/Light.h"
#include "texture/Texture2D.h"
#include "fb/LocalFB.h"
#include "mpi/fb/DistributedFrameBuffer.h"
#include "transferFunction/TransferFunction.h"
#include "common/OSPWork.h"
#include "ospcommon/utility/getEnvVar.h"
// std
#include <algorithm>

#ifdef OPEN_MPI
# include <thread>
//# define _GNU_SOURCE
# include <sched.h>
#endif

#ifdef _WIN32
#  include <windows.h> // for Sleep and gethostname
#  include <process.h> // for getpid
#else
#  include <unistd.h> // for gethostname
#endif

#ifndef HOST_NAME_MAX
#  define HOST_NAME_MAX 10000
#endif

namespace ospray {
  namespace mpi {

    using namespace mpicommon;
    using ospcommon::utility::getEnvVar;

    static void embreeErrorFunc(void *, const RTCError code, const char* str)
    {
      std::stringstream msg;
      msg << "#osp: Embree internal error " << code << " : " << str;
      postStatusMsg(msg);
      throw std::runtime_error(msg.str());
    }

    std::unique_ptr<work::Work> readWork(work::WorkTypeRegistry &registry,
                                         networking::ReadStream &readStream)
    {
      work::Work::tag_t tag;
      readStream >> tag;

      static size_t numWorkReceived = 0;
      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#osp.mpi.worker: got work #" << numWorkReceived++
          << ", tag " << tag;

      auto make_work = registry.find(tag);
      if (make_work == registry.end()) {
        std::stringstream msg;
        msg << "Invalid work type received - tag #: " << tag << "\n";
        postStatusMsg(msg);
        throw std::runtime_error(msg.str());
      }

      auto work = make_work->second();

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL) << ": " << typeString(work);

      work->deserialize(readStream);
      return work;
    }

    /*! it's up to the proper init
      routine to decide which processes call this function and which
      ones don't. This function will not return.

      \internal We ssume that mpi::worker and mpi::app have already been set up
    */
    void runWorker()
    {
      auto &device = ospray::api::Device::current;

      // NOTE(jda) - This guard guarentees that the embree device gets cleaned
      //             up no matter how the scope of runWorker() is left
      struct EmbreeDeviceScopeGuard
      {
        RTCDevice embreeDevice;
        ~EmbreeDeviceScopeGuard() { rtcDeleteDevice(embreeDevice); }
      };

      auto embreeDevice =
          rtcNewDevice(generateEmbreeDeviceCfg(*device).c_str());
      device->embreeDevice = embreeDevice;
      EmbreeDeviceScopeGuard guard;
      guard.embreeDevice = embreeDevice;

      rtcDeviceSetErrorFunction2(embreeDevice, embreeErrorFunc, nullptr);

      if (rtcDeviceGetError(embreeDevice) != RTC_NO_ERROR) {
        // why did the error function not get called !?
        postStatusMsg() << "#osp:init: embree internal error number "
                        << (int)rtcDeviceGetError(embreeDevice);
      }

      char hostname[HOST_NAME_MAX];
      gethostname(hostname,HOST_NAME_MAX);
      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#w: running MPI worker process " << worker.rank
          << "/" << worker.size << " on pid " << getpid() << "@" << hostname;

      // -------------------------------------------------------
      // setting up read/write streams
      // -------------------------------------------------------
      auto mpiFabric  = make_unique<MPIBcastFabric>(mpi::app, MPI_ROOT, 0);
      auto readStream = make_unique<networking::BufferedReadStream>(*mpiFabric);

      // create registry of work item types
      std::map<work::Work::tag_t,work::CreateWorkFct> workTypeRegistry;
      work::registerOSPWorkItems(workTypeRegistry);

      while (1) {
        auto work = readWork(workTypeRegistry, *readStream);
        postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
            << "#osp.mpi.worker: processing work " << typeIdOf(work)
            << ": " << typeString(work);

        work->run();

        postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
            << "#osp.mpi.worker: done w/ work " << typeIdOf(work)
            << ": " << typeString(work);
      }
    }

  } // ::ospray::mpi
} // ::ospray
