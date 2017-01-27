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
#include "mpiCommon/async/CommLayer.h"
#include "mpi/MPIDevice.h"
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
#include "mpi/render/MPILoadBalancer.h"
#include "transferFunction/TransferFunction.h"
#include "common/OSPWork.h"
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
void sleep(unsigned int seconds)
{
    Sleep(seconds * 1000);
}
#else
#  include <unistd.h> // for gethostname
#endif

#define DBG(a) /**/

#ifndef HOST_NAME_MAX
#  define HOST_NAME_MAX 10000
#endif

namespace ospray {

  namespace mpi {
    using std::cout;
    using std::endl;

    extern bool logMPI;

    OSPRAY_MPI_INTERFACE void runWorker();

    void embreeErrorFunc(const RTCError code, const char* str)
    {
      std::cerr << "#osp: embree internal error " << code << " : "
                << str << std::endl;
      throw std::runtime_error("embree internal error '"+std::string(str)+"'");
    }

    std::shared_ptr<work::Work> readWork(std::map<work::Work::tag_t,work::CreateWorkFct> &registry,
                                         std::shared_ptr<ReadStream> &readStream)
    {
      work::Work::tag_t tag;
      *readStream >> tag;

      static size_t numWorkReceived = 0;
      if(logMPI)
        printf("#osp.mpi.worker: got work #%li, tag %i: %s\n",
               numWorkReceived++,
               tag,work::commandTagToString((work::CommandTag)tag).c_str());
      
      auto make_work = registry.find(tag);
      assert(make_work != registry.end());
      std::shared_ptr<work::Work> work = make_work->second();
      assert(work);
      work->deserialize(*readStream);
      return work;
    }

    bool checkIfWeNeedToDoMPIDebugOutputs()
    {
      char *envVar = getenv("OSPRAY_MPI_DEBUG");
      if (!envVar) return false;
      PING; PRINT(envVar);
      return atoi(envVar) > 0;
    }
    
    void checkIfThisIsOpenMPIAndIfSoTurnAllCPUsBackOn();
    
    /*! it's up to the proper init
      routine to decide which processes call this function and which
      ones don't. This function will not return.

      \internal We ssume that mpi::worker and mpi::app have already been set up
    */
    void runWorker()
    {
      checkIfThisIsOpenMPIAndIfSoTurnAllCPUsBackOn();
      
      auto &device = ospray::api::Device::current;

      auto numThreads = device ? device->numThreads : -1;
      
      // initialize embree. (we need to do this here rather than in
      // ospray::init() because in mpi-mode the latter is also called
      // in the host-stubs, where it shouldn't.
      std::stringstream embreeConfig;
      if (device && device->debugMode)
        embreeConfig << " threads=1,verbose=2";
      else if(numThreads > 0)
        embreeConfig << " threads=" << numThreads;

      // NOTE(jda) - This guard guarentees that the embree device gets cleaned
      //             up no matter how the scope of runWorker() is left
      struct EmbreeDeviceScopeGuard {
        RTCDevice embreeDevice;
        ~EmbreeDeviceScopeGuard() { rtcDeleteDevice(embreeDevice); }
      };

      RTCDevice embreeDevice = rtcNewDevice(embreeConfig.str().c_str());
      device->embreeDevice = embreeDevice;
      EmbreeDeviceScopeGuard guard;
      guard.embreeDevice = embreeDevice;

      rtcDeviceSetErrorFunction(embreeDevice, embreeErrorFunc);

      if (rtcDeviceGetError(embreeDevice) != RTC_NO_ERROR) {
        // why did the error function not get called !?
        std::cerr << "#osp:init: embree internal error number "
                  << (int)rtcDeviceGetError(embreeDevice) << std::endl;
      }

      char hostname[HOST_NAME_MAX];
      gethostname(hostname,HOST_NAME_MAX);
      printf("#w: running MPI worker process %i/%i on pid %i@%s\n",
             worker.rank,worker.size,getpid(),hostname);

      TiledLoadBalancer::instance = make_unique<staticLoadBalancer::Slave>();

      // -------------------------------------------------------
      // setting up read/write streams
      // -------------------------------------------------------
      std::shared_ptr<Fabric> mpiFabric
        = std::make_shared<MPIBcastFabric>(mpi::app);
      std::shared_ptr<ReadStream> readStream
        = std::make_shared<BufferedFabric::ReadStream>(mpiFabric);
      std::shared_ptr<WriteStream> writeStream
        = std::make_shared<BufferedFabric::WriteStream>(mpiFabric);

      // create registry of work item types
      std::map<work::Work::tag_t,work::CreateWorkFct> workTypeRegistry;
      work::registerOSPWorkItems(workTypeRegistry);

      logMPI = checkIfWeNeedToDoMPIDebugOutputs();
      while (1) {
        std::shared_ptr<work::Work> work = readWork(workTypeRegistry,readStream);
        if (logMPI)
          std::cout << "#osp.mpi.worker: processing work "
                    << work::commandTagToString((work::CommandTag)work->getTag()) << std::endl;
        work->run();
        if (logMPI)
          std::cout << "#osp.mpi.worker: done w/ work "
                    << work::commandTagToString((work::CommandTag)work->getTag()) << std::endl;
      }
    }


    
  } // ::ospray::mpi
} // ::ospray
