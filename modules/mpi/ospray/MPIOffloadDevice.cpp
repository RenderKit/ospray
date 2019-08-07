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

#include "MPIOffloadDevice.h"
// ospray
#include "common/Messaging.h"
#include "common/QueryableTask.h"
// ospcommon
#include "ospcommon/utility/getEnvVar.h"
// maml
#include "maml/maml.h"

#ifdef OPEN_MPI
#include <sched.h>
#include <thread>
#endif

namespace ospray {
  namespace mpi {

    using namespace mpicommon;
    using ospcommon::utility::getEnvVar;

    ///////////////////////////////////////////////////////////////////////////
    // Forward declarations ///////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    //! this runs an ospray worker process.
    /*! it's up to the proper init routine to decide which processes
      call this function and which ones don't. This function will not
      return. */
    void runWorker();

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
      MPI_CALL(Comm_split(mpi::world.comm, 1, mpi::world.rank, &app.comm));

      app.makeIntraComm();

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#w: app process " << app.rank << '/' << app.size << " (global "
          << world.rank << '/' << world.size;

      MPI_CALL(Intercomm_create(app.comm, 0, world.comm, 1, 1, &worker.comm));

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "master: Made 'worker' intercomm (through intercomm_create): "
          << std::hex << std::showbase << worker.comm << std::noshowbase
          << std::dec;

      worker.makeInterComm();

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
      MPI_CALL(Comm_split(mpi::world.comm, 0, mpi::world.rank, &worker.comm));

      worker.makeIntraComm();

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "master: Made 'worker' intercomm (through split): " << std::hex
          << std::showbase << worker.comm << std::noshowbase << std::dec;

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#w: app process " << app.rank << '/' << app.size << " (global "
          << world.rank << '/' << world.size;

      MPI_CALL(Intercomm_create(worker.comm, 0, world.comm, 0, 1, &app.comm));

      app.makeInterComm();

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

    void createMPI_RanksBecomeWorkers()
    {
      int ac           = 2;
      const char *av[] = {"ospray_mpi_worker", "--osp:mpi"};
      mpi::init(&ac, av, true);

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#o: initMPI::OSPonRanks: " << world.rank << '/' << world.size;

      // Note: here we don't run this collective through MAML, since we
      // haven't started it yet.
      MPI_CALL(Barrier(world.comm));

      throwIfNotMpiParallel();

      if (IamTheMaster())
        setupMaster();
      else {
        setupWorker();

        // now, all workers will enter their worker loop (ie, they will *not*
        // return)
        runWorker();
        throw std::runtime_error("should never reach here!");
        /* no return here - 'runWorker' will never return */
      }
    }

    ///////////////////////////////////////////////////////////////////////////
    // MPIDevice definitions //////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    MPIOffloadDevice::~MPIOffloadDevice()
    {
      if (IamTheMaster()) {
        postStatusMsg("shutting down mpi device", OSPRAY_MPI_VERBOSE_LEVEL);
        work::CommandFinalize work;
        processWork(work, true);
      }
    }

    void MPIOffloadDevice::initializeDevice()
    {
      Device::commit();

      initialized = true;

      createMPI_RanksBecomeWorkers();

      if (mpi::world.size != 1) {
        if (mpi::world.rank < 0) {
          throw std::runtime_error(
              "OSPRay MPI startup error. Use \"mpirun "
              "-n 1 <command>\" when calling an "
              "application that tries to spawn to start "
              "the application you were trying to "
              "start.");
        }
      }

      work::registerOSPWorkItems(workRegistry);

      // Only the master running at this point, workers go on to
      // MPIOffloadWorker::runWorker
      auto OSPRAY_FORCE_COMPRESSION =
          utility::getEnvVar<int>("OSPRAY_FORCE_COMPRESSION");
      // Turning on the compression past 64 ranks seems to be a good
      // balancing point for cost of compressing vs. performance gain
      auto enableCompression = OSPRAY_FORCE_COMPRESSION.value_or(
          mpicommon::numGlobalRanks() >= OSP_MPI_COMPRESSION_THRESHOLD);

      maml::init(enableCompression);
      messaging::init(world);
      maml::start();

      /* set up fabric and stuff - by now all the communicators should
         be properly set up */
      mpiFabric   = make_unique<MPIBcastFabric>(mpi::worker, MPI_ROOT, 0);
      readStream  = make_unique<networking::BufferedReadStream>(*mpiFabric);
      writeStream = make_unique<networking::BufferedWriteStream>(*mpiFabric);
    }

    ///////////////////////////////////////////////////////////////////////////
    // ManagedObject Implementation ///////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    void MPIOffloadDevice::commit()
    {
      if (!initialized)
        initializeDevice();

      auto OSPRAY_DYNAMIC_LOADBALANCER =
          getEnvVar<int>("OSPRAY_DYNAMIC_LOADBALANCER");

      auto useDynamicLoadBalancer = getParam<int>(
          "dynamicLoadBalancer", OSPRAY_DYNAMIC_LOADBALANCER.value_or(false));

      auto OSPRAY_PREALLOCATED_TILES =
          utility::getEnvVar<int>("OSPRAY_PREALLOCATED_TILES");

      auto preAllocatedTiles = OSPRAY_PREALLOCATED_TILES.value_or(
          getParam<int>("preAllocatedTiles", 4));

      auto OSPRAY_MPI_OFFLOAD_WRITE_BUFFER_SCALE =
          utility::getEnvVar<float>("OSPRAY_MPI_OFFLOAD_WRITE_BUFFER_SCALE");

      auto writeBufferSize = OSPRAY_MPI_OFFLOAD_WRITE_BUFFER_SCALE.value_or(
          getParam<float>("writeBufferScale", 1.f));

      size_t bufferSize = 1024 * size_t(writeBufferSize * 1024);

      writeStream->flush();
      writeStream =
          make_unique<networking::BufferedWriteStream>(*mpiFabric, bufferSize);

      work::SetLoadBalancer slbWork(
          ObjectHandle(), useDynamicLoadBalancer, preAllocatedTiles);
      processWork(slbWork);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Device Implementation //////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    int MPIOffloadDevice::loadModule(const char *name)
    {
      work::LoadModule work(name);
      processWork(work, true);
      return work.errorCode;
    }

    ///////////////////////////////////////////////////////////////////////////
    // OSPRay Data Arrays /////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPData MPIOffloadDevice::newData(size_t nitems,
                                      OSPDataType format,
                                      const void *init,
                                      int flags)
    {
      ObjectHandle handle = allocateHandle();

      work::NewData work(handle, nitems, format, init, flags);
      processWork(work);

      return (OSPData)(int64)handle;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Renderable Objects /////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPLight MPIOffloadDevice::newLight(const char *type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewLight work(type, handle);
      processWork(work);
      return (OSPLight)(int64)handle;
    }

    OSPCamera MPIOffloadDevice::newCamera(const char *type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewCamera work(type, handle);
      processWork(work);
      return (OSPCamera)(int64)handle;
    }

    OSPGeometry MPIOffloadDevice::newGeometry(const char *type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewGeometry work(type, handle);
      processWork(work);
      return (OSPGeometry)(int64)handle;
    }

    OSPVolume MPIOffloadDevice::newVolume(const char *type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewVolume work(type, handle);
      processWork(work);
      return (OSPVolume)(int64)handle;
    }

    OSPGeometricModel MPIOffloadDevice::newGeometricModel(OSPGeometry geom)
    {
      ObjectHandle handle = allocateHandle();
      work::NewGeometricModel work(handle, (ObjectHandle &)geom);
      processWork(work);
      return (OSPGeometricModel)(int64)handle;
    }

    OSPVolumetricModel MPIOffloadDevice::newVolumetricModel(OSPVolume volume)
    {
      ObjectHandle handle = allocateHandle();
      work::NewVolumetricModel work(handle, (ObjectHandle &)volume);
      processWork(work);
      return (OSPVolumetricModel)(int64)handle;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Model Meta-Data ////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPMaterial MPIOffloadDevice::newMaterial(const char *renderer_type,
                                              const char *material_type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewMaterial work(renderer_type, material_type, handle);
      processWork(work);
      return (OSPMaterial)(int64)handle;
    }

    OSPTransferFunction MPIOffloadDevice::newTransferFunction(const char *type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewTransferFunction work(type, handle);
      processWork(work);
      return (OSPTransferFunction)(int64)handle;
    }

    OSPTexture MPIOffloadDevice::newTexture(const char *type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewTexture work(type, handle);
      processWork(work);
      return (OSPTexture)(int64)handle;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Instancing /////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPGroup MPIOffloadDevice::newGroup()
    {
      ObjectHandle handle = allocateHandle();
      work::NewGroup work("", handle);
      processWork(work);
      return (OSPGroup)(int64)handle;
    }

    OSPInstance MPIOffloadDevice::newInstance(OSPGroup group)
    {
      ObjectHandle handle = allocateHandle();
      work::NewInstance work(handle, (ObjectHandle &)group);
      processWork(work);
      return (OSPInstance)(int64)handle;
    }

    ///////////////////////////////////////////////////////////////////////////
    // World Manipulation /////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPWorld MPIOffloadDevice::newWorld()
    {
      ObjectHandle handle = allocateHandle();
      work::NewWorld work("", handle);
      processWork(work);
      return (OSPWorld)(int64)handle;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Object Parameters //////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    void MPIOffloadDevice::setString(OSPObject _object,
                                     const char *bufName,
                                     const char *s)
    {
      work::SetParam<std::string> work((ObjectHandle &)_object, bufName, s);
      processWork(work);
    }

    void MPIOffloadDevice::setObject(OSPObject _target,
                                     const char *bufName,
                                     OSPObject _value)
    {
      work::SetParam<OSPObject> work((ObjectHandle &)_target, bufName, _value);
      processWork(work);
    }

    void MPIOffloadDevice::setBool(OSPObject _object,
                                   const char *bufName,
                                   const bool b)
    {
      work::SetParam<bool> work((ObjectHandle &)_object, bufName, b);
      processWork(work);
    }

    void MPIOffloadDevice::setFloat(OSPObject _object,
                                    const char *bufName,
                                    const float f)
    {
      work::SetParam<float> work((ObjectHandle &)_object, bufName, f);
      processWork(work);
    }

    void MPIOffloadDevice::setInt(OSPObject _object,
                                  const char *bufName,
                                  const int i)
    {
      work::SetParam<int> work((ObjectHandle &)_object, bufName, i);
      processWork(work);
    }

    void MPIOffloadDevice::setVec2f(OSPObject _object,
                                    const char *bufName,
                                    const vec2f &v)
    {
      work::SetParam<vec2f> work((ObjectHandle &)_object, bufName, v);
      processWork(work);
    }

    void MPIOffloadDevice::setVec2i(OSPObject _object,
                                    const char *bufName,
                                    const vec2i &v)
    {
      work::SetParam<vec2i> work((ObjectHandle &)_object, bufName, v);
      processWork(work);
    }

    void MPIOffloadDevice::setVec3f(OSPObject _object,
                                    const char *bufName,
                                    const vec3f &v)
    {
      work::SetParam<vec3f> work((ObjectHandle &)_object, bufName, v);
      processWork(work);
    }

    void MPIOffloadDevice::setVec3i(OSPObject _object,
                                    const char *bufName,
                                    const vec3i &v)
    {
      work::SetParam<vec3i> work((ObjectHandle &)_object, bufName, v);
      processWork(work);
    }

    void MPIOffloadDevice::setVec4f(OSPObject _object,
                                    const char *bufName,
                                    const vec4f &v)
    {
      work::SetParam<vec4f> work((ObjectHandle &)_object, bufName, v);
      processWork(work);
    }

    void MPIOffloadDevice::setVec4i(OSPObject _object,
                                    const char *bufName,
                                    const vec4i &v)
    {
      work::SetParam<vec4i> work((ObjectHandle &)_object, bufName, v);
      processWork(work);
    }

    void MPIOffloadDevice::setBox1f(OSPObject _object,
                                    const char *bufName,
                                    const box1f &v)
    {
      work::SetParam<box1f> work((ObjectHandle &)_object, bufName, v);
      processWork(work);
    }

    void MPIOffloadDevice::setBox1i(OSPObject _object,
                                    const char *bufName,
                                    const box1i &v)
    {
      work::SetParam<box1i> work((ObjectHandle &)_object, bufName, v);
      processWork(work);
    }

    void MPIOffloadDevice::setBox2f(OSPObject _object,
                                    const char *bufName,
                                    const box2f &v)
    {
      work::SetParam<box2f> work((ObjectHandle &)_object, bufName, v);
      processWork(work);
    }

    void MPIOffloadDevice::setBox2i(OSPObject _object,
                                    const char *bufName,
                                    const box2i &v)
    {
      work::SetParam<box2i> work((ObjectHandle &)_object, bufName, v);
      processWork(work);
    }

    void MPIOffloadDevice::setBox3f(OSPObject _object,
                                    const char *bufName,
                                    const box3f &v)
    {
      work::SetParam<box3f> work((ObjectHandle &)_object, bufName, v);
      processWork(work);
    }

    void MPIOffloadDevice::setBox3i(OSPObject _object,
                                    const char *bufName,
                                    const box3i &v)
    {
      work::SetParam<box3i> work((ObjectHandle &)_object, bufName, v);
      processWork(work);
    }

    void MPIOffloadDevice::setBox4f(OSPObject _object,
                                    const char *bufName,
                                    const box4f &v)
    {
      work::SetParam<box4f> work((ObjectHandle &)_object, bufName, v);
      processWork(work);
    }

    void MPIOffloadDevice::setBox4i(OSPObject _object,
                                    const char *bufName,
                                    const box4i &v)
    {
      work::SetParam<box4i> work((ObjectHandle &)_object, bufName, v);
      processWork(work);
    }

    void MPIOffloadDevice::setLinear3f(OSPObject _object,
                                       const char *bufName,
                                       const linear3f &v)
    {
      work::SetParam<linear3f> work((ObjectHandle &)_object, bufName, v);
      processWork(work);
    }

    void MPIOffloadDevice::setAffine3f(OSPObject _object,
                                       const char *bufName,
                                       const affine3f &v)
    {
      work::SetParam<affine3f> work((ObjectHandle &)_object, bufName, v);
      processWork(work);
    }

    void MPIOffloadDevice::setVoidPtr(OSPObject _object,
                                      const char *bufName,
                                      void *v)
    {
      UNUSED(_object, bufName, v);
      throw std::runtime_error(
          "setting a void pointer as parameter to an "
          "object is not allowed in MPI mode");
    }

    ///////////////////////////////////////////////////////////////////////////
    // Object + Parameter Lifetime Management /////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    void MPIOffloadDevice::commit(OSPObject _object)
    {
      const ObjectHandle handle = (const ObjectHandle &)_object;
      work::CommitObject work(handle);
      processWork(work);
    }

    void MPIOffloadDevice::removeParam(OSPObject object, const char *name)
    {
      work::RemoveParam work((ObjectHandle &)object, name);
      processWork(work);
    }

    void MPIOffloadDevice::release(OSPObject _obj)
    {
      work::CommandRelease work((const ObjectHandle &)_obj);
      processWork(work);
    }

    ///////////////////////////////////////////////////////////////////////////
    // FrameBuffer Manipulation ///////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPFrameBuffer MPIOffloadDevice::frameBufferCreate(
        const vec2i &size,
        const OSPFrameBufferFormat mode,
        const uint32 channels)
    {
      ObjectHandle handle = allocateHandle();
      work::CreateFrameBuffer work(handle, size, mode, channels);
      processWork(work, true);
      return (OSPFrameBuffer)(int64)handle;
    }

    OSPImageOp MPIOffloadDevice::newImageOp(const char *type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewImageOp work(type, handle);
      processWork(work);
      return (OSPImageOp)(int64)handle;
    }

    const void *MPIOffloadDevice::frameBufferMap(OSPFrameBuffer _fb,
                                                 OSPFrameBufferChannel channel)
    {
      ObjectHandle handle = (const ObjectHandle &)_fb;
      FrameBuffer *fb     = (FrameBuffer *)handle.lookup();

      return fb->mapBuffer(channel);
    }

    void MPIOffloadDevice::frameBufferUnmap(const void *mapped,
                                            OSPFrameBuffer _fb)
    {
      ObjectHandle handle = (const ObjectHandle &)_fb;
      FrameBuffer *fb     = (FrameBuffer *)handle.lookup();

      fb->unmap(mapped);
    }

    float MPIOffloadDevice::getVariance(OSPFrameBuffer _fb)
    {
      auto fbHandle = (ObjectHandle &)_fb;
      auto *fb      = (FrameBuffer *)fbHandle.lookup();
      return fb->getVariance();
    }

    void MPIOffloadDevice::resetAccumulation(OSPFrameBuffer _fb)
    {
      work::ResetAccumulation work(_fb);
      processWork(work);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Frame Rendering ////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPRenderer MPIOffloadDevice::newRenderer(const char *type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewRenderer work(type, handle);
      processWork(work);
      return (OSPRenderer)(int64)handle;
    }

    float MPIOffloadDevice::renderFrame(OSPFrameBuffer _fb,
                                        OSPRenderer _renderer,
                                        OSPCamera _camera,
                                        OSPWorld _world)
    {
      OSPFuture _future = renderFrameAsync(_fb, _renderer, _camera, _world);
      wait(_future, OSP_FRAME_FINISHED);
      return getVariance(_fb);
    }

    OSPFuture MPIOffloadDevice::renderFrameAsync(OSPFrameBuffer _fb,
                                                 OSPRenderer _renderer,
                                                 OSPCamera _camera,
                                                 OSPWorld _world)
    {
      // When using the offload device the user's application won't be
      // calling MPI at all, so in this case we don't need to worry about
      // hitting issues with thread multiple vs. thread serialized
      ObjectHandle futureHandle = allocateHandle();
      work::RenderFrameAsync work(
          _fb, _renderer, _camera, _world, futureHandle);
      processWork(work, true);
      return (OSPFuture)(int64)futureHandle;
    }

    int MPIOffloadDevice::isReady(OSPFuture _task, OSPSyncEvent event)
    {
      auto handle = (ObjectHandle &)_task;
      auto *task  = (QueryableTask *)handle.lookup();
      return task->isFinished(event);
    }

    void MPIOffloadDevice::wait(OSPFuture _task, OSPSyncEvent event)
    {
      auto handle = (ObjectHandle &)_task;
      auto *task  = (QueryableTask *)handle.lookup();
      task->wait(event);
    }

    void MPIOffloadDevice::cancel(OSPFuture _task)
    {
      auto handle = (ObjectHandle &)_task;
      auto *task  = (QueryableTask *)handle.lookup();
      return task->cancel();
    }

    float MPIOffloadDevice::getProgress(OSPFuture _task)
    {
      auto handle = (ObjectHandle &)_task;
      auto *task  = (QueryableTask *)handle.lookup();
      return task->getProgress();
    }

    OSPPickResult MPIOffloadDevice::pick(OSPFrameBuffer fb,
                                         OSPRenderer renderer,
                                         OSPCamera camera,
                                         OSPWorld world,
                                         const vec2f &screenPos)
    {
      work::Pick work(fb, renderer, camera, world, screenPos);
      processWork(work, true);
      return work.pickResult;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Misc Functions /////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    void MPIOffloadDevice::processWork(work::Work &work, bool flushWriteStream)
    {
      static size_t numWorkSent = 0;
      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#osp.mpi.master: processing/sending work item " << numWorkSent++;

      auto tag = typeIdOf(work);
      writeStream->write(&tag, sizeof(tag));
      work.serialize(*writeStream);

      if (flushWriteStream)
        writeStream->flush();

      // Run the master side variant of the work unit
      work.runOnMaster();

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#osp.mpi.master: done work item, tag " << tag << ": "
          << typeString(work);
    }

    ObjectHandle MPIOffloadDevice::allocateHandle() const
    {
      return ObjectHandle();
    }

    OSP_REGISTER_DEVICE(MPIOffloadDevice, mpi_offload);
    OSP_REGISTER_DEVICE(MPIOffloadDevice, mpi);

  }  // namespace mpi
}  // namespace ospray

extern "C" OSPRAY_DLLEXPORT void ospray_init_module_mpi() {}
