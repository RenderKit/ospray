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

#ifndef _WIN32
#include <dlfcn.h>
#include <unistd.h>
#endif

#ifdef OPEN_MPI
#include <sched.h>
#include <thread>
#endif

#include "MPIOffloadDevice.h"
#include "camera/Camera.h"
#include "common/Data.h"
#include "common/Library.h"
#include "common/MPIBcastFabric.h"
#include "common/MPICommon.h"
#include "common/OSPWork.h"
#include "common/SocketBcastFabric.h"
#include "common/Util.h"
#include "common/World.h"
#include "fb/DistributedFrameBuffer.h"
#include "fb/LocalFB.h"
#include "geometry/TriangleMesh.h"
#include "ospcommon/networking/DataStreaming.h"
#include "ospcommon/networking/Socket.h"
#include "ospcommon/utility/ArrayView.h"
#include "ospcommon/utility/OwnedArray.h"
#include "ospcommon/utility/getEnvVar.h"
#include "render/DistributedLoadBalancer.h"
#include "render/RenderTask.h"
#include "render/Renderer.h"
#include "volume/Volume.h"

namespace ospray {
  namespace mpi {

    using namespace mpicommon;
    using namespace ospcommon;

    void parseHost(const std::string &hostport, std::string &host, int &port)
    {
      auto fnd = hostport.find(':');
      if (fnd == std::string::npos) {
        throw std::runtime_error("failed to parse host: " + hostport);
      }
      host = hostport.substr(0, fnd);
      port = std::stoi(hostport.substr(fnd + 1));
    }

    ///////////////////////////////////////////////////////////////////////////
    // Forward declarations ///////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    //! this runs an ospray worker process.
    /*! it's up to the proper init routine to decide which processes
      call this function and which ones don't. This function will not
      return. */
    void runWorker(bool useMPIFabric);

    ///////////////////////////////////////////////////////////////////////////
    // Misc helper functions //////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    static inline void throwIfNotMpiParallel()
    {
      if (world.size <= 1) {
        throw std::runtime_error(
            "No MPI workers found.\n#osp:mpi: Fatal Error "
            "- OSPRay told to run in MPI mode, but there "
            "seems to be no MPI peers!?\n#osp:mpi: (Did "
            "you forget an 'mpirun' in front of your "
            "application?)");
      }
    }

    static inline void setupMaster()
    {
      MPI_Comm appComm;
      MPI_CALL(Comm_split(world.comm, 1, world.rank, &appComm));

      int size = 0;
      int rank = 0;
      MPI_Comm_size(appComm, &size);
      MPI_Comm_rank(appComm, &rank);

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#w: app process " << rank << '/' << size << " (global "
          << world.rank << '/' << world.size << ')';

      MPI_CALL(Barrier(world.comm));

      // -------------------------------------------------------
      // at this point, all processes should be set up and synced. in
      // particular:
      // - app has intracommunicator to all workers (and vica versa)
      // - app process(es) are in one intercomm ("app"); workers all in
      //   another ("worker")
      // - all processes (incl app) have barrier'ed, and thus now in sync.
    }

    static inline void setupWorker()
    {
      MPI_CALL(Comm_split(world.comm, 0, world.rank, &worker.comm));

      worker.makeIntraComm();

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "master: Made 'worker' intercomm (through split): " << std::hex
          << std::showbase << worker.comm << std::noshowbase << std::dec;

      MPI_CALL(Barrier(world.comm));

      // -------------------------------------------------------
      // at this point, all processes should be set up and synced. in
      // particular:
      // - app has intracommunicator to all workers (and vica versa)
      // - app process(es) are in one intercomm ("app"); workers all in
      //   another ("worker")
      // - all processes (incl app) have barrier'ed, and thus now in sync.
    }

    ///////////////////////////////////////////////////////////////////////////
    // MPI initialization helper functions ////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    /*! TODO WILL: Update these comments
     in this mode ("ospray on ranks" mode, or "ranks" mode), the
        user has launched the app across all ranks using mpirun "<all
        rank> <app>"; no new processes need to get launched.

        Based on the 'startworkers' flag, this function can set up ospray in
        one of two modes:

        in "workers" mode (startworkers=true) all ranks > 0 become
        workers, and will NOT return to the application; rank 0 is the
        master that controls those workers but doesn't do any
        rendeirng (we may at some point allow the master to join in
        working as well, but currently this is not implemented). to
        reach that mode we call this function with
        'startworkers=true', which will make sure that, even though
        all ranks _called_ mpiinit, only rank 0 will ever return to
        the app, while all other ranks will automatically go to
        running worker code, and never ever return from this function.

        b) in "distributed" mode the app itself is distributed, and
        will use the ospray distributed api to control ospray in a
        data-distributed mode. in this way, we'll call this function
        with startWorkers=false, which will let all ranks return from
        this function to do further work in the app.

        For this function, we assume:

        - all *all* MPI_COMM_WORLD processes are going into this function

        - this fct is called from ospInit (with ranksBecomeWorkers=true) or
          from ospdMpiInit (w/ ranksBecomeWorkers = false)
    */
    void createMPI_RanksBecomeWorkers(int *ac, const char **av)
    {
      mpi::init(ac, av, true);

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#o: initMPI::OSPonRanks: " << world.rank << '/' << world.size;

      // Note: here we don't run this collective through MAML, since we
      // haven't started it yet.
      MPI_CALL(Barrier(world.comm));

      throwIfNotMpiParallel();

      if (world.rank == 0)
        setupMaster();
      else {
        setupWorker();

        // now, all workers will enter their worker loop (ie, they will *not*
        // return)
        mpi::runWorker(true);
        throw std::runtime_error("should never reach here!");
        /* no return here - 'runWorker' will never return */
      }
    }

    void createMPI_ListenForClient(int *ac, const char **av)
    {
      mpi::init(ac, av, true);

      if (world.rank == 0) {
        postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
            << "#o: Initialize OSPRay MPI in 'Listen for Client' Mode\n";
      }
      worker.comm = world.comm;
      worker.makeIntraComm();

      mpi::runWorker(false);
    }

    ///////////////////////////////////////////////////////////////////////////
    // MPIDevice definitions //////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    MPIOffloadDevice::~MPIOffloadDevice()
    {
      // TODO: This test needs to be corrected for handlign offload w/ sockets
      // in connect/accept mode.
      if (dynamic_cast<MPIFabric *>(fabric.get()) && world.rank == 0) {
        postStatusMsg("shutting down mpi device", OSPRAY_MPI_VERBOSE_LEVEL);

        networking::BufferWriter writer;
        writer << work::FINALIZE;
        sendWork(writer.buffer);

        // TODO: if not mpi, don't finalize on head node
        MPI_Finalize();
      }
    }

    void MPIOffloadDevice::initializeDevice()
    {
      Device::commit();

      initialized = true;

      int _ac           = 2;
      const char *_av[] = {"ospray_mpi_worker", "--osp:mpi"};

      std::string mode = getParam<std::string>("mpiMode", "mpi");

      if (mode == "mpi") {
        createMPI_RanksBecomeWorkers(&_ac, _av);
        // Only the master returns from this call
        fabric = make_unique<MPIFabric>(world, 0);
        maml::init(false);
        maml::start();
      } else if (mode == "mpi-listen") {
        createMPI_ListenForClient(&_ac, _av);
      } else if (mode == "mpi-connect") {
        std::string hostPort = getParam<std::string>("host", "");

        if (hostPort.empty()) {
          throw std::runtime_error(
              "Error: mpi-connect requires a host:port "
              "argument to connect to");
        }
        std::string host;
        int port = 0;
        parseHost(hostPort, host, port);
        postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
            << "MPIOffloadDevice connecting to " << host << ":" << port << "\n";
        fabric = make_unique<SocketWriterFabric>(host, port);
      } else {
        throw std::runtime_error("Invalid MPI mode!");
      }

      // The collectives don't get compressed through the optional
      // compression path in MAML, so we can leave this off.
      // TODO: We may want to optionally enable compression for collectives,
      // if we have a lot of ranks
    }

    ///////////////////////////////////////////////////////////////////////////
    // ManagedObject Implementation ///////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    void MPIOffloadDevice::commit()
    {
      if (!initialized)
        initializeDevice();

        // TODO: These params should be shipped over to the workers
#if 0
      auto OSPRAY_DYNAMIC_LOADBALANCER =
          utility::getEnvVar<int>("OSPRAY_DYNAMIC_LOADBALANCER");

      auto useDynamicLoadBalancer = getParam<int>(
          "dynamicLoadBalancer", OSPRAY_DYNAMIC_LOADBALANCER.value_or(false));

      auto OSPRAY_PREALLOCATED_TILES =
          utility::getEnvVar<int>("OSPRAY_PREALLOCATED_TILES");

      auto preAllocatedTiles = OSPRAY_PREALLOCATED_TILES.value_or(
          getParam<int>("preAllocatedTiles", 4));
#endif

      // writeStream = mpiFabric.get();
      // writeStream->flush();
      // writeStream =
      // make_unique<networking::BufferedWriteStream>(*mpiFabric, bufferSize);

      /*
      work::SetLoadBalancer slbWork(
          ObjectHandle(), useDynamicLoadBalancer, preAllocatedTiles);
      processWork(slbWork);
      */
    }

    ///////////////////////////////////////////////////////////////////////////
    // Device Implementation //////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    int MPIOffloadDevice::loadModule(const char *name)
    {
      networking::BufferWriter writer;
      writer << work::LOAD_MODULE << std::string(name);
      sendWork(writer.buffer);

      // TODO: Error reporting from the workers
      return 0;
    }

    ///////////////////////////////////////////////////////////////////////////
    // OSPRay Data Arrays /////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPData MPIOffloadDevice::newData(size_t nitems,
                                      OSPDataType format,
                                      const void *init,
                                      int flags)
    {
      if (init == nullptr) {
        WarnOnce warn(
            "Making uninitialized OSPData on MPIOffloadDevice, "
            "this is likely an error");
      }

      ObjectHandle handle = allocateHandle();

      networking::BufferWriter writer;
      writer << work::NEW_DATA << handle.i64 << nitems << format << flags;
      sendWork(writer.buffer);

      std::shared_ptr<utility::AbstractArray<uint8_t>> dataView;
      if (flags & OSP_DATA_SHARED_BUFFER) {
        dataView = std::make_shared<utility::ArrayView<uint8_t>>(
            const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(init)),
            sizeOf(format) * nitems);
      } else {
        dataView = std::make_shared<utility::OwnedArray<uint8_t>>(
            const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(init)),
            sizeOf(format) * nitems);
      }

      fabric->sendBcast(dataView);

      return (OSPData)(int64)handle;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Renderable Objects /////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPLight MPIOffloadDevice::newLight(const char *type)
    {
      ObjectHandle handle = allocateHandle();

      networking::BufferWriter writer;
      writer << work::NEW_LIGHT << handle.i64 << std::string(type);
      sendWork(writer.buffer);

      return (OSPLight)(int64)handle;
    }

    OSPCamera MPIOffloadDevice::newCamera(const char *type)
    {
      ObjectHandle handle = allocateHandle();

      networking::BufferWriter writer;
      writer << work::NEW_CAMERA << handle.i64 << std::string(type);
      sendWork(writer.buffer);

      return (OSPCamera)(int64)handle;
    }

    OSPGeometry MPIOffloadDevice::newGeometry(const char *type)
    {
      ObjectHandle handle = allocateHandle();

      networking::BufferWriter writer;
      writer << work::NEW_GEOMETRY << handle.i64 << std::string(type);
      sendWork(writer.buffer);

      return (OSPGeometry)(int64)handle;
    }

    OSPVolume MPIOffloadDevice::newVolume(const char *type)
    {
      ObjectHandle handle = allocateHandle();

      networking::BufferWriter writer;
      writer << work::NEW_VOLUME << handle.i64 << std::string(type);
      sendWork(writer.buffer);

      return (OSPVolume)(int64)handle;
    }

    OSPGeometricModel MPIOffloadDevice::newGeometricModel(OSPGeometry geom)
    {
      ObjectHandle handle = allocateHandle();

      ObjectHandle geomHandle = (ObjectHandle &)geom;
      networking::BufferWriter writer;
      writer << work::NEW_GEOMETRIC_MODEL << handle.i64 << geomHandle.i64;
      sendWork(writer.buffer);

      return (OSPGeometricModel)(int64)handle;
    }

    OSPVolumetricModel MPIOffloadDevice::newVolumetricModel(OSPVolume volume)
    {
      ObjectHandle handle = allocateHandle();

      ObjectHandle volHandle = (ObjectHandle &)volume;
      networking::BufferWriter writer;
      writer << work::NEW_VOLUMETRIC_MODEL << handle.i64 << volHandle.i64;
      sendWork(writer.buffer);

      return (OSPVolumetricModel)(int64)handle;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Model Meta-Data ////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPMaterial MPIOffloadDevice::newMaterial(const char *renderer_type,
                                              const char *material_type)
    {
      ObjectHandle handle = allocateHandle();

      networking::BufferWriter writer;
      writer << work::NEW_MATERIAL << handle.i64 << std::string(renderer_type)
             << std::string(material_type);
      sendWork(writer.buffer);

      return (OSPMaterial)(int64)handle;
    }

    OSPTransferFunction MPIOffloadDevice::newTransferFunction(const char *type)
    {
      ObjectHandle handle = allocateHandle();

      networking::BufferWriter writer;
      writer << work::NEW_TRANSFER_FUNCTION << handle.i64 << std::string(type);
      sendWork(writer.buffer);

      return (OSPTransferFunction)(int64)handle;
    }

    OSPTexture MPIOffloadDevice::newTexture(const char *type)
    {
      ObjectHandle handle = allocateHandle();

      networking::BufferWriter writer;
      writer << work::NEW_TEXTURE << handle.i64 << std::string(type);
      sendWork(writer.buffer);

      return (OSPTexture)(int64)handle;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Instancing /////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPGroup MPIOffloadDevice::newGroup()
    {
      ObjectHandle handle = allocateHandle();

      networking::BufferWriter writer;
      writer << work::NEW_GROUP << handle.i64;
      sendWork(writer.buffer);

      return (OSPGroup)(int64)handle;
    }

    OSPInstance MPIOffloadDevice::newInstance(OSPGroup group)
    {
      ObjectHandle handle      = allocateHandle();
      ObjectHandle groupHandle = (ObjectHandle &)group;

      networking::BufferWriter writer;
      writer << work::NEW_INSTANCE << handle.i64 << groupHandle.i64;
      sendWork(writer.buffer);

      return (OSPInstance)(int64)handle;
    }

    ///////////////////////////////////////////////////////////////////////////
    // World Manipulation /////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPWorld MPIOffloadDevice::newWorld()
    {
      ObjectHandle handle = allocateHandle();

      networking::BufferWriter writer;
      writer << work::NEW_WORLD << handle.i64;
      sendWork(writer.buffer);

      return (OSPWorld)(int64)handle;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Object Parameters //////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    void MPIOffloadDevice::setString(OSPObject _object,
                                     const char *bufName,
                                     const char *s)
    {
      setParam<std::string>((ObjectHandle &)_object,
                            bufName,
                            std::string(s),
                            work::SET_PARAM_STRING);
    }

    void MPIOffloadDevice::setObject(OSPObject _target,
                                     const char *bufName,
                                     OSPObject _value)
    {
      setParam<OSPObject>(
          (ObjectHandle &)_target, bufName, _value, work::SET_PARAM_OBJECT);
    }

    void MPIOffloadDevice::setBool(OSPObject _object,
                                   const char *bufName,
                                   const bool b)
    {
      setParam<bool>((ObjectHandle &)_object, bufName, b, work::SET_PARAM_BOOL);
    }

    void MPIOffloadDevice::setFloat(OSPObject _object,
                                    const char *bufName,
                                    const float f)
    {
      setParam<float>(
          (ObjectHandle &)_object, bufName, f, work::SET_PARAM_FLOAT);
    }

    void MPIOffloadDevice::setInt(OSPObject _object,
                                  const char *bufName,
                                  const int i)
    {
      setParam<int>((ObjectHandle &)_object, bufName, i, work::SET_PARAM_INT);
    }

    void MPIOffloadDevice::setVec2f(OSPObject _object,
                                    const char *bufName,
                                    const vec2f &v)
    {
      setParam<vec2f>(
          (ObjectHandle &)_object, bufName, v, work::SET_PARAM_VEC2F);
    }

    void MPIOffloadDevice::setVec2i(OSPObject _object,
                                    const char *bufName,
                                    const vec2i &v)
    {
      setParam<vec2i>(
          (ObjectHandle &)_object, bufName, v, work::SET_PARAM_VEC2I);
    }

    void MPIOffloadDevice::setVec3f(OSPObject _object,
                                    const char *bufName,
                                    const vec3f &v)
    {
      setParam<vec3f>(
          (ObjectHandle &)_object, bufName, v, work::SET_PARAM_VEC3F);
    }

    void MPIOffloadDevice::setVec3i(OSPObject _object,
                                    const char *bufName,
                                    const vec3i &v)
    {
      setParam<vec3i>(
          (ObjectHandle &)_object, bufName, v, work::SET_PARAM_VEC3I);
    }

    void MPIOffloadDevice::setVec4f(OSPObject _object,
                                    const char *bufName,
                                    const vec4f &v)
    {
      setParam<vec4f>(
          (ObjectHandle &)_object, bufName, v, work::SET_PARAM_VEC4F);
    }

    void MPIOffloadDevice::setVec4i(OSPObject _object,
                                    const char *bufName,
                                    const vec4i &v)
    {
      setParam<vec4i>(
          (ObjectHandle &)_object, bufName, v, work::SET_PARAM_VEC4I);
    }

    void MPIOffloadDevice::setBox1f(OSPObject _object,
                                    const char *bufName,
                                    const box1f &v)
    {
      setParam<box1f>(
          (ObjectHandle &)_object, bufName, v, work::SET_PARAM_BOX1F);
    }

    void MPIOffloadDevice::setBox1i(OSPObject _object,
                                    const char *bufName,
                                    const box1i &v)
    {
      setParam<box1i>(
          (ObjectHandle &)_object, bufName, v, work::SET_PARAM_BOX1I);
    }

    void MPIOffloadDevice::setBox2f(OSPObject _object,
                                    const char *bufName,
                                    const box2f &v)
    {
      setParam<box2f>(
          (ObjectHandle &)_object, bufName, v, work::SET_PARAM_BOX2F);
    }

    void MPIOffloadDevice::setBox2i(OSPObject _object,
                                    const char *bufName,
                                    const box2i &v)
    {
      setParam<box2i>(
          (ObjectHandle &)_object, bufName, v, work::SET_PARAM_BOX2I);
    }

    void MPIOffloadDevice::setBox3f(OSPObject _object,
                                    const char *bufName,
                                    const box3f &v)
    {
      setParam<box3f>(
          (ObjectHandle &)_object, bufName, v, work::SET_PARAM_BOX3F);
    }

    void MPIOffloadDevice::setBox3i(OSPObject _object,
                                    const char *bufName,
                                    const box3i &v)
    {
      setParam<box3i>(
          (ObjectHandle &)_object, bufName, v, work::SET_PARAM_BOX3I);
    }

    void MPIOffloadDevice::setBox4f(OSPObject _object,
                                    const char *bufName,
                                    const box4f &v)
    {
      setParam<box4f>(
          (ObjectHandle &)_object, bufName, v, work::SET_PARAM_BOX4F);
    }

    void MPIOffloadDevice::setBox4i(OSPObject _object,
                                    const char *bufName,
                                    const box4i &v)
    {
      setParam<box4i>(
          (ObjectHandle &)_object, bufName, v, work::SET_PARAM_BOX4I);
    }

    void MPIOffloadDevice::setLinear3f(OSPObject _object,
                                       const char *bufName,
                                       const linear3f &v)
    {
      setParam<linear3f>(
          (ObjectHandle &)_object, bufName, v, work::SET_PARAM_LINEAR3F);
    }

    void MPIOffloadDevice::setAffine3f(OSPObject _object,
                                       const char *bufName,
                                       const affine3f &v)
    {
      setParam<affine3f>(
          (ObjectHandle &)_object, bufName, v, work::SET_PARAM_AFFINE3F);
    }

    void MPIOffloadDevice::setVoidPtr(OSPObject _object,
                                      const char *bufName,
                                      void *v)
    {
      UNUSED(_object, bufName, v);
      throw std::runtime_error(
          "setting a void pointer as parameter to an "
          "object is not valid in MPIOffload");
    }

    ///////////////////////////////////////////////////////////////////////////
    // Object + Parameter Lifetime Management /////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    void MPIOffloadDevice::commit(OSPObject _object)
    {
      const ObjectHandle handle = (const ObjectHandle &)_object;

      networking::BufferWriter writer;
      writer << work::COMMIT << handle.i64;
      sendWork(writer.buffer);
    }

    void MPIOffloadDevice::removeParam(OSPObject _object, const char *name)
    {
      const ObjectHandle handle = (const ObjectHandle &)_object;

      networking::BufferWriter writer;
      writer << work::REMOVE_PARAM << handle.i64 << std::string(name);
      sendWork(writer.buffer);
    }

    void MPIOffloadDevice::release(OSPObject _object)
    {
      const ObjectHandle handle = (const ObjectHandle &)_object;

      networking::BufferWriter writer;
      writer << work::RELEASE << handle.i64;
      sendWork(writer.buffer);

      // TODO: On the head node we should also clear this handle
    }

    ///////////////////////////////////////////////////////////////////////////
    // FrameBuffer Manipulation ///////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPFrameBuffer MPIOffloadDevice::frameBufferCreate(
        const vec2i &size,
        const OSPFrameBufferFormat format,
        const uint32 channels)
    {
      ObjectHandle handle = allocateHandle();

      networking::BufferWriter writer;
      writer << work::CREATE_FRAMEBUFFER << handle.i64 << size
             << (uint32_t)format << channels;
      sendWork(writer.buffer);
      return (OSPFrameBuffer)(int64)handle;
    }

    OSPImageOp MPIOffloadDevice::newImageOp(const char *type)
    {
      ObjectHandle handle = allocateHandle();

      networking::BufferWriter writer;
      writer << work::NEW_IMAGE_OP << handle.i64 << std::string(type);
      sendWork(writer.buffer);
      return (OSPImageOp)(int64)handle;
    }

    const void *MPIOffloadDevice::frameBufferMap(OSPFrameBuffer _fb,
                                                 OSPFrameBufferChannel channel)
    {
      using namespace utility;

      ObjectHandle handle = (ObjectHandle &)_fb;
      networking::BufferWriter writer;
      writer << work::MAP_FRAMEBUFFER << handle.i64 << (uint32_t)channel;
      sendWork(writer.buffer);

      uint64_t nbytes = 0;
      auto bytesView  = ArrayView<uint8_t>(reinterpret_cast<uint8_t *>(&nbytes),
                                          sizeof(nbytes));

      fabric->recv(bytesView, rootWorkerRank());

      auto mapping = ospcommon::make_unique<OwnedArray<uint8_t>>();
      mapping->resize(nbytes, 0);
      fabric->recv(*mapping, rootWorkerRank());

      void *ptr                       = mapping->data();
      framebufferMappings[handle.i64] = std::move(mapping);

      return ptr;
    }

    void MPIOffloadDevice::frameBufferUnmap(const void *, OSPFrameBuffer _fb)
    {
      ObjectHandle handle = (ObjectHandle &)_fb;
      auto fnd            = framebufferMappings.find(handle.i64);
      if (fnd != framebufferMappings.end()) {
        framebufferMappings.erase(fnd);
      }
    }

    float MPIOffloadDevice::getVariance(OSPFrameBuffer _fb)
    {
      using namespace utility;

      ObjectHandle handle = (ObjectHandle &)_fb;
      networking::BufferWriter writer;
      writer << work::GET_VARIANCE << handle.i64;
      sendWork(writer.buffer);

      float variance = 0;
      auto view = ArrayView<uint8_t>(reinterpret_cast<uint8_t *>(&variance),
                                     sizeof(variance));

      fabric->recv(view, rootWorkerRank());
      return variance;
    }

    void MPIOffloadDevice::resetAccumulation(OSPFrameBuffer _fb)
    {
      ObjectHandle handle = (ObjectHandle &)_fb;
      networking::BufferWriter writer;
      writer << work::RESET_ACCUMULATION << handle.i64;
      sendWork(writer.buffer);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Frame Rendering ////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPRenderer MPIOffloadDevice::newRenderer(const char *type)
    {
      ObjectHandle handle = allocateHandle();

      networking::BufferWriter writer;
      writer << work::NEW_RENDERER << handle.i64 << std::string(type);
      sendWork(writer.buffer);
      return (OSPRenderer)(int64)handle;
    }

    float MPIOffloadDevice::renderFrame(OSPFrameBuffer _fb,
                                        OSPRenderer _renderer,
                                        OSPCamera _camera,
                                        OSPWorld _world)
    {
      OSPFuture _future = renderFrameAsync(_fb, _renderer, _camera, _world);
      wait(_future, OSP_FRAME_FINISHED);
      release(_future);
      return getVariance(_fb);
    }

    OSPFuture MPIOffloadDevice::renderFrameAsync(OSPFrameBuffer _fb,
                                                 OSPRenderer _renderer,
                                                 OSPCamera _camera,
                                                 OSPWorld _world)
    {
      ObjectHandle futureHandle         = allocateHandle();
      const ObjectHandle fbHandle       = (ObjectHandle &)_fb;
      const ObjectHandle rendererHandle = (ObjectHandle &)_renderer;
      const ObjectHandle cameraHandle   = (ObjectHandle &)_camera;
      const ObjectHandle worldHandle    = (ObjectHandle &)_world;

      networking::BufferWriter writer;
      writer << work::RENDER_FRAME_ASYNC << fbHandle << rendererHandle
             << cameraHandle << worldHandle << futureHandle;
      sendWork(writer.buffer);
      return (OSPFuture)(int64)futureHandle;
    }

    int MPIOffloadDevice::isReady(OSPFuture _task, OSPSyncEvent event)
    {
      const ObjectHandle handle = (ObjectHandle &)_task;
      networking::BufferWriter writer;
      writer << work::FUTURE_IS_READY << handle.i64 << (uint32_t)event;
      sendWork(writer.buffer);
      int result = 0;
      utility::ArrayView<uint8_t> view(reinterpret_cast<uint8_t *>(&result),
                                       sizeof(result));
      fabric->recv(view, rootWorkerRank());
      return result;
    }

    void MPIOffloadDevice::wait(OSPFuture _task, OSPSyncEvent event)
    {
      const ObjectHandle handle = (ObjectHandle &)_task;
      networking::BufferWriter writer;
      writer << work::FUTURE_WAIT << handle.i64 << (uint32_t)event;
      sendWork(writer.buffer);
      int result = 0;
      utility::ArrayView<uint8_t> view(reinterpret_cast<uint8_t *>(&result),
                                       sizeof(result));
      fabric->recv(view, rootWorkerRank());
    }

    void MPIOffloadDevice::cancel(OSPFuture _task)
    {
      const ObjectHandle handle = (ObjectHandle &)_task;
      networking::BufferWriter writer;
      writer << work::FUTURE_CANCEL << handle.i64;
      sendWork(writer.buffer);
    }

    float MPIOffloadDevice::getProgress(OSPFuture _task)
    {
      const ObjectHandle handle = (ObjectHandle &)_task;
      networking::BufferWriter writer;
      writer << work::FUTURE_GET_PROGRESS << handle.i64;
      sendWork(writer.buffer);
      float result = 0;
      utility::ArrayView<uint8_t> view(reinterpret_cast<uint8_t *>(&result),
                                       sizeof(result));
      fabric->recv(view, rootWorkerRank());
      return result;
    }

    OSPPickResult MPIOffloadDevice::pick(OSPFrameBuffer fb,
                                         OSPRenderer renderer,
                                         OSPCamera camera,
                                         OSPWorld world,
                                         const vec2f &screenPos)
    {
      const ObjectHandle fbHandle       = (ObjectHandle &)fb;
      const ObjectHandle rendererHandle = (ObjectHandle &)renderer;
      const ObjectHandle cameraHandle   = (ObjectHandle &)camera;
      const ObjectHandle worldHandle    = (ObjectHandle &)world;

      networking::BufferWriter writer;
      writer << work::PICK << fbHandle << rendererHandle << cameraHandle
             << worldHandle << screenPos;
      sendWork(writer.buffer);

      OSPPickResult result = {0};
      utility::ArrayView<uint8_t> view(reinterpret_cast<uint8_t *>(&result),
                                       sizeof(OSPPickResult));
      fabric->recv(view, rootWorkerRank());
      return result;
    }

    void MPIOffloadDevice::sendWork(
        std::shared_ptr<utility::AbstractArray<uint8_t>> work)
    {
      networking::BufferWriter header;
      header << (uint64_t)work->size();

      fabric->sendBcast(header.buffer);
      fabric->sendBcast(work);
    }

    int MPIOffloadDevice::rootWorkerRank() const
    {
      if (dynamic_cast<SocketWriterFabric *>(fabric.get()))
        return 0;

      // TODO: we may also want to support MPI intercomm setups w/ comm
      // accept/connect?
      return 1;
    }

    ObjectHandle MPIOffloadDevice::allocateHandle() const
    {
      return ObjectHandle();
    }

    OSP_REGISTER_DEVICE(MPIOffloadDevice, mpi_offload);
    OSP_REGISTER_DEVICE(MPIOffloadDevice, mpi);

  }  // namespace mpi
}  // namespace ospray
