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

// std
#include <algorithm>

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

    OSPRAY_MPI_INTERFACE void runWorker();

    void embreeErrorFunc(const RTCError code, const char* str)
    {
      std::cerr << "#osp: embree internal error " << code << " : "
                << str << std::endl;
      throw std::runtime_error("embree internal error '"+std::string(str)+"'");
    }


    /*! it's up to the proper init
      routine to decide which processes call this function and which
      ones don't. This function will not return.

      \internal We ssume that mpi::worker and mpi::app have already been set up
    */
    void runWorker()
    {
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

      auto bufferedComm = mpi::BufferedMPIComm::get();
      while (1) {
        std::vector<work::Work*> workCommands;
        bufferedComm->recv(mpi::Address(&mpi::app, (int32)mpi::RECV_ALL),
                           workCommands);
        for (work::Work *&w : workCommands) {
          w->run();
          delete w;
          w = nullptr;
        }
      }
    }

  } // ::ospray::mpi
} // ::ospray
