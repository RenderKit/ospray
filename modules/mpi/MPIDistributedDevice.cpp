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

#undef NDEBUG // do all assertions in this file

#include "mpiCommon/MPICommon.h"
#include "mpi/MPIDistributedDevice.h"
#include "mpi/render/MPILoadBalancer.h"

#ifdef OPEN_MPI
# include <thread>
//# define _GNU_SOURCE
# include <sched.h>
#endif

namespace ospray {
  namespace mpi {

    // Forward declarations ///////////////////////////////////////////////////

    bool checkIfWeNeedToDoMPIDebugOutputs();

    // Misc helper functions //////////////////////////////////////////////////

    static inline void throwIfNotMpiParallel()
    {
      if (world.size <= 1) {
        throw std::runtime_error("No MPI workers found.\n#osp:mpi: Fatal Error "
                                 "- OSPRay told to run in MPI mode, but there "
                                 "seems to be no MPI peers!?\n#osp:mpi: (Did "
                                 "you forget an 'mpirun' in front of your "
                                 "application?)");
      }
    }

    static inline void setupMaster()
    {
      SERIALIZED_MPI_CALL(
        Comm_split(mpi::world.comm,1,mpi::world.rank,&app.comm)
      );

      app.makeIntraComm();

      if (logMPI) {
        postErrorMsg(OSPRAY_MPI_VERBOSE_LEVEL)
            << "#w: app process " << app.rank << '/' << app.size
            << " (global " << world.rank << '/' << world.size;
      }

      SERIALIZED_MPI_CALL(
        Intercomm_create(app.comm, 0, world.comm, 1, 1, &worker.comm)
      );

      if (logMPI) {
        postErrorMsg(OSPRAY_MPI_VERBOSE_LEVEL)
            << "master: Made 'worker' intercomm (through intercomm_create): "
            << std::hex << std::showbase << worker.comm
            << std::noshowbase << std::dec;
      }

      worker.makeInterComm();
    }

    static inline void setupWorker()
    {
      SERIALIZED_MPI_CALL(
        Comm_split(mpi::world.comm,0,mpi::world.rank,&worker.comm)
      );

      worker.makeIntraComm();

      if (logMPI) {
        postErrorMsg(OSPRAY_MPI_VERBOSE_LEVEL)
            << "master: Made 'worker' intercomm (through split): "
            << std::hex << std::showbase << worker.comm
            << std::noshowbase << std::dec;

        postErrorMsg(OSPRAY_MPI_VERBOSE_LEVEL)
            << "#w: app process " << app.rank << '/' << app.size
            << " (global " << world.rank << '/' << world.size;
      }

      SERIALIZED_MPI_CALL(
        Intercomm_create(worker.comm, 0, world.comm, 0, 1, &app.comm)
      );

      app.makeInterComm();
    }

    // MPIDistributedDevice definitions ///////////////////////////////////////
    
    MPIDistributedDevice::MPIDistributedDevice()
    {
      // check if env variables etc compel us to do logging...
      logMPI = checkIfWeNeedToDoMPIDebugOutputs();
    }

    MPIDistributedDevice::~MPIDistributedDevice()
    {
      //TODO
    }

    void MPIDistributedDevice::commit()
    {
      if (initialized)// NOTE (jda) - doesn't make sense to commit again?
        return;

      Device::commit();

      initialized = true;

      if (mpi::world.size != 1) {
        if (mpi::world.rank < 0) {
          throw std::runtime_error("OSPRay MPI startup error. Use \"mpirun "
                                   "-n 1 <command>\" when calling an "
                                   "application that tries to spawn to start "
                                   "the application you were trying to "
                                   "start.");
        }
      }

      /* set up fabric and stuff - by now all the communicators should
         be properly set up */
      mpiFabric   = make_unique<MPIBcastFabric>(mpi::worker);
      readStream  = make_unique<BufferedFabric::ReadStream>(*mpiFabric);
      writeStream = make_unique<BufferedFabric::WriteStream>(*mpiFabric);

      TiledLoadBalancer::instance = make_unique<staticLoadBalancer::Master>();
    }

    OSPFrameBuffer 
    MPIDistributedDevice::frameBufferCreate(const vec2i &size,
                                            const OSPFrameBufferFormat mode,
                                            const uint32 channels)
    {
      NOT_IMPLEMENTED;
    }


    const void*
    MPIDistributedDevice::frameBufferMap(OSPFrameBuffer _fb,
                                         OSPFrameBufferChannel channel)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::frameBufferUnmap(const void *mapped,
                                                OSPFrameBuffer _fb)
    {
      NOT_IMPLEMENTED;
    }

    OSPModel MPIDistributedDevice::newModel()
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::commit(OSPObject _object)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::addGeometry(OSPModel _model,
                                           OSPGeometry _geometry)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::addVolume(OSPModel _model, OSPVolume _volume)
    {
      NOT_IMPLEMENTED;
    }

    OSPData MPIDistributedDevice::newData(size_t nitems, OSPDataType format,
                                          void *init, int flags)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setVoidPtr(OSPObject _object,
                                          const char *bufName,
                                          void *v)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::removeParam(OSPObject object, const char *name)
    {
      NOT_IMPLEMENTED;
    }

    int MPIDistributedDevice::setRegion(OSPVolume _volume, const void *source,
                                        const vec3i &index, const vec3i &count)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setString(OSPObject _object,
                                         const char *bufName,
                                         const char *s)
    {
      NOT_IMPLEMENTED;
    }

    int MPIDistributedDevice::loadModule(const char *name)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setFloat(OSPObject _object,
                                        const char *bufName,
                                        const float f)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setInt(OSPObject _object,
                                      const char *bufName,
                                      const int i)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setVec2f(OSPObject _object,
                                        const char *bufName,
                                        const vec2f &v)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setVec3f(OSPObject _object,
                                        const char *bufName,
                                        const vec3f &v)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setVec4f(OSPObject _object,
                                        const char *bufName,
                                        const vec4f &v)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setVec2i(OSPObject _object,
                                        const char *bufName,
                                        const vec2i &v)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setVec3i(OSPObject _object,
                                        const char *bufName,
                                        const vec3i &v)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setObject(OSPObject _target,
                                         const char *bufName,
                                         OSPObject _value)
    {
      NOT_IMPLEMENTED;
    }

    OSPPixelOp MPIDistributedDevice::newPixelOp(const char *type)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setPixelOp(OSPFrameBuffer _fb, OSPPixelOp _op)
    {
      NOT_IMPLEMENTED;
    }

    OSPRenderer MPIDistributedDevice::newRenderer(const char *type)
    {
      NOT_IMPLEMENTED;
    }

    OSPCamera MPIDistributedDevice::newCamera(const char *type)
    {
      NOT_IMPLEMENTED;
    }

    OSPVolume MPIDistributedDevice::newVolume(const char *type)
    {
      NOT_IMPLEMENTED;
    }

    OSPGeometry MPIDistributedDevice::newGeometry(const char *type)
    {
      NOT_IMPLEMENTED;
    }

    OSPMaterial MPIDistributedDevice::newMaterial(OSPRenderer _renderer,
                                                  const char *type)
    {
      NOT_IMPLEMENTED;
    }

    OSPTransferFunction
    MPIDistributedDevice::newTransferFunction(const char *type)
    {
      NOT_IMPLEMENTED;
    }

    OSPLight MPIDistributedDevice::newLight(OSPRenderer _renderer,
                                            const char *type)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::frameBufferClear(OSPFrameBuffer _fb,
                                     const uint32 fbChannelFlags)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::removeGeometry(OSPModel _model,
                                              OSPGeometry _geometry)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::removeVolume(OSPModel _model, OSPVolume _volume)
    {
      NOT_IMPLEMENTED;
    }

    float MPIDistributedDevice::renderFrame(OSPFrameBuffer _fb,
                                 OSPRenderer _renderer,
                                 const uint32 fbChannelFlags)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::release(OSPObject _obj)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setMaterial(OSPGeometry _geometry,
                                           OSPMaterial _material)
    {
      NOT_IMPLEMENTED;
    }

    OSPTexture2D MPIDistributedDevice::newTexture2D(const vec2i &sz,
        const OSPTextureFormat type, void *data, const uint32 flags)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::sampleVolume(float **results,
                                 OSPVolume volume,
                                 const vec3f *worldCoordinates,
                                 const size_t &count)
    {
      NOT_IMPLEMENTED;
    }

    ObjectHandle MPIDistributedDevice::allocateHandle() const
    {
      if (currentApiMode != OSPD_MODE_MASTERED)
        throw std::runtime_error("Can only alloc handles in MASTERED mode!");

      return ObjectHandle();
    }

    OSP_REGISTER_DEVICE(MPIDistributedDevice, mpi_distributed_device);
    OSP_REGISTER_DEVICE(MPIDistributedDevice, mpi_distributed);

  } // ::ospray::mpi
} // ::ospray
