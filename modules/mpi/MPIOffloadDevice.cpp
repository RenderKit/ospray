// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
#include "mpiCommon/MPIBcastFabric.h"
#include "mpi/MPIOffloadDevice.h"
#include "common/Model.h"
#include "common/Data.h"
#include "common/Library.h"
#include "common/Util.h"
#include "ospcommon/sysinfo.h"
#include "ospcommon/FileName.h"
#include "geometry/TriangleMesh.h"
#include "render/Renderer.h"
#include "camera/Camera.h"
#include "volume/Volume.h"
#include "mpi/render/MPILoadBalancer.h"
#include "fb/LocalFB.h"
#include "mpi/fb/DistributedFrameBuffer.h"
#include "ospcommon/networking/BufferedDataStreaming.h"
#include "ospcommon/networking/Socket.h"
#include "ospcommon/utility/getEnvVar.h"

// std
#ifndef _WIN32
#  include <unistd.h> // for fork()
#  include <dlfcn.h>
#endif

#ifdef OPEN_MPI
# include <thread>
//# define _GNU_SOURCE
# include <sched.h>
#endif

namespace ospray {
  namespace mpi {

    using namespace mpicommon;
    using ospcommon::utility::getEnvVar;

    // Forward declarations ///////////////////////////////////////////////////

    //! this runs an ospray worker process.
    /*! it's up to the proper init routine to decide which processes
      call this function and which ones don't. This function will not
      return. */
    void runWorker();

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
      MPI_CALL(Comm_split(mpi::world.comm,1,mpi::world.rank,&app.comm));

      app.makeIntraComm();

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#w: app process " << app.rank << '/' << app.size
          << " (global " << world.rank << '/' << world.size;

      MPI_CALL(Intercomm_create(app.comm, 0, world.comm, 1, 1, &worker.comm));

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "master: Made 'worker' intercomm (through intercomm_create): "
          << std::hex << std::showbase << worker.comm
          << std::noshowbase << std::dec;

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
      MPI_CALL(Comm_split(mpi::world.comm,0,mpi::world.rank,&worker.comm));

      worker.makeIntraComm();

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "master: Made 'worker' intercomm (through split): "
          << std::hex << std::showbase << worker.comm
          << std::noshowbase << std::dec;

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#w: app process " << app.rank << '/' << app.size
          << " (global " << world.rank << '/' << world.size;

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

    // MPI initialization helper functions ////////////////////////////////////

    /*! in this mode ("ospray on ranks" mode, or "ranks" mode), the
        user has launched the app across all ranks using mpirun "<all
        rank> <app>"; no new processes need to get launched.

        Based on the 'startworkers' flag, this function can set up ospray in
        one of two modes:

        in "workers" mode (startworkes=true) all ranks > 0 become
        workers, and will NOT return to the application; rank 0 is the
        master that controls those workers but doesn't do any
        rendeirng (we may at some point allow the master to join in
        working as well, but currently this is not implemented). to
        reach that mode we call this function with
        'startworkers=true', which will make sure that, even though
        all ranks _called_ mpiinit, only rank 0 will ever return to
        the app, while all other ranks will automatically go to
        running worker code, and never ever return from this function.

        b) in "distribtued" mode the app itself is distributed, and
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
      mpi::init(ac,av,true);

      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#o: initMPI::OSPonRanks: " << world.rank << '/' << world.size;

      MPI_CALL(Barrier(world.comm));

      throwIfNotMpiParallel();

      if (IamTheMaster())
        setupMaster();
      else {
        setupWorker();

        // now, all workers will enter their worker loop (ie, they will *not*
        // return)
        mpi::runWorker();
        throw std::runtime_error("should never reach here!");
        /* no return here - 'runWorker' will never return */
      }
    }

    /*! in this mode ("separate worker group" mode)
      - the user may or may not have launched MPI explicitly for his app
      - the app may or may not be running distributed
      - the ospray frontend (the part linked to the app) will wait for a remote
        MPI group of workers to connect to this app
      - the ospray frontend will store the port its waiting on connections for
        in the filename passed to this function; or will print it to stdout if
        this is nullptr
    */
    void createMPI_ListenForWorkers(int *ac, const char **av)
    {
      mpi::init(ac,av,true);

      if (world.rank < 1) {
        postStatusMsg("====================================================\n"
                      "initializing OSPRay MPI in 'listen for workers' mode\n"
                      "====================================================",
                      OSPRAY_MPI_VERBOSE_LEVEL);
      }

      app.comm = world.comm;
      app.makeIntraComm();

      char appPortName[MPI_MAX_PORT_NAME];

      if (!IamTheMaster()) {
        throw std::runtime_error("--osp:mpi-listen only makes sense with "
                                 "a single rank...");
      }

      MPI_CALL(Open_port(MPI_INFO_NULL, appPortName));

      socket_t mpiPortSocket = ospcommon::bind(3141);
      socket_t clientSocket = ospcommon::listen(mpiPortSocket);
      size_t len = strlen(appPortName);
      ospcommon::write(clientSocket,&len,sizeof(len));
      ospcommon::write(clientSocket,appPortName,len);
      flush(clientSocket);

      MPI_CALL(Comm_accept(appPortName,MPI_INFO_NULL,0,app.comm,&worker.comm));
      ospcommon::close(clientSocket);
      ospcommon::close(mpiPortSocket);

      worker.makeInterComm();

      mpi::Group mergedComm;
      MPI_CALL(Intercomm_merge(worker.comm,0,&mergedComm.comm));
      mergedComm.makeIntraComm();
      mpi::world.comm = mergedComm.comm;
      mpi::world.makeIntraComm();

      if (app.rank == 0) {
        postStatusMsg("=====================================================\n"
                      "OSPRAY Worker ring connected\n"
                      "=====================================================",
                      OSPRAY_MPI_VERBOSE_LEVEL);
      }

      mpi::world.barrier();
    }

    void createMPI_connectToListener(int *ac, const char **av,
                                     const std::string &host)
    {
      mpi::init(ac,av,true);

      if (world.rank < 1) {
        postStatusMsg("=====================================================\n"
                      "initializing OSPRay MPI in 'connect to master' mode  \n"
                      "=====================================================",
                      OSPRAY_MPI_VERBOSE_LEVEL);
      }

      worker.comm = world.comm;
      worker.makeIntraComm();

      char appPortName[MPI_MAX_PORT_NAME];
      if (worker.rank == 0) {
        auto masterSocket = ospcommon::connect(host.c_str(),3141);

        size_t len;
        ospcommon::read(masterSocket,&len,sizeof(len));
        ospcommon::read(masterSocket,appPortName,len);
        ospcommon::close(masterSocket);
      }

      MPI_CALL(Comm_connect(appPortName,MPI_INFO_NULL,0,worker.comm,&app.comm));
      app.makeInterComm();


      mpi::Group mergedComm;
      MPI_CALL(Intercomm_merge(app.comm,1,&mergedComm.comm));
      mergedComm.makeIntraComm();
      mpi::world.comm = mergedComm.comm;
      mpi::world.makeIntraComm();

      mpi::world.barrier();

      postStatusMsg("starting worker...", OSPRAY_MPI_VERBOSE_LEVEL);
      mpi::runWorker();
    }

    /*! in this mode ("separate worker group" mode)
      - the user may or may not have launched MPI explicitly for his app
      - the app may or may not be running distributed
      - the ospray frontend (the part linked to the app) will use the specified
        'launchCmd' to launch a _new_ MPI group of worker processes.
      - the ospray frontend will assume the launched process to output
        'OSPRAY_SERVICE_PORT' stdout, will parse that output for this string,
        and create an mpi connection to this port to establish the service
    */
    void createMPI_LaunchWorkerGroup(int *ac, const char **av,
                                     const char *launchCommand)
    {
      mpi::init(ac,av,true);

      Assert(launchCommand);

      MPI_CALL(Comm_dup(world.comm,&app.comm));
      app.makeIntraComm();

      char appPortName[MPI_MAX_PORT_NAME];
      if (app.rank == 0 || app.size == -1) {
        postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
            << "=======================================================\n"
            << "initializing OSPRay MPI in 'launching new workers' mode"
            << "=======================================================\n"
            << "using launch script '" << launchCommand << "'";

        MPI_CALL(Open_port(MPI_INFO_NULL, appPortName));

        // fix port name: replace all '$'s by '%'s to allow using it on the
        //                cmdline...
        char *fixedPortName = strdup(appPortName);

        postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
            << "with port " << fixedPortName
            << "\n=======================================================";

        for (char *s = fixedPortName; *s; ++s)
          if (*s == '$') *s = '%';

        char systemCommand[10000];
        sprintf(systemCommand,"%s %s",launchCommand,fixedPortName);
#ifdef _WIN32
        throw std::runtime_error("OSPRay MPI: no fork() yet on Windows");
#else
        if (fork()) {
          auto result = system(systemCommand);
          (void)result;

          postStatusMsg("OSPRay worker process has died - killing application");
          exit(0);
        }
#endif
      }

      MPI_CALL(Comm_accept(appPortName,MPI_INFO_NULL,0,app.comm,&worker.comm));
      worker.makeInterComm();

      if (app.rank == 0 || app.size == -1) {
        postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
            << "OSPRay MPI Worker ring successfully connected.\n"
            << "found " << worker.size << " workers."
            << "\n=======================================================";
      }

      if (app.size > 1) {
        if (app.rank == 1) {
          postStatusMsg()
              << "ospray:WARNING: you're trying to run an mpi-parallel app "
              << "with ospray\n"
              << "(only the root node is allowed to issue ospray api "
              << "calls right now)\n"
              << "=======================================================";
        }

        MPI_CALL(Barrier(app.comm));
      }

      MPI_CALL(Barrier(app.comm));
    }

    // MPIDevice definitions //////////////////////////////////////////////////

    MPIOffloadDevice::MPIOffloadDevice()
    {
      maml::init();
    }

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

      work::registerOSPWorkItems(workRegistry);

      int _ac = 2;
      const char *_av[] = {"ospray_mpi_worker", "--osp:mpi"};

      std::string mode = getParam<std::string>("mpiMode", "mpi");

      if (mode == "mpi") {
        createMPI_RanksBecomeWorkers(&_ac,_av);
      } else if(mode == "mpi-launch") {
        std::string launchCommand = getParam<std::string>("launchCommand", "");

        if (launchCommand.empty()) {
          throw std::runtime_error("You must provide the launchCommand "
                                   "parameter in mpi-launch mode!");
        }

        createMPI_LaunchWorkerGroup(&_ac,_av,launchCommand.c_str());
      } else if (mode == "mpi-listen") {
        createMPI_ListenForWorkers(&_ac,_av);
      } else if (mode == "mpi-connect") {
        std::string portName =
            getParam<std::string>("portName", "");

        if (portName.empty()) {
          throw std::runtime_error("You must provide the port name string "
                                   "where the master is listening at!");
        }

        createMPI_connectToListener(&_ac,_av,portName);
      } else {
        throw std::runtime_error("Invalid MPI mode!");
      }

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
      mpiFabric   = make_unique<MPIBcastFabric>(mpi::worker, MPI_ROOT, 0);
      readStream  = make_unique<networking::BufferedReadStream>(*mpiFabric);
      writeStream = make_unique<networking::BufferedWriteStream>(*mpiFabric);
    }

    void MPIOffloadDevice::commit()
    {
      if (!initialized)
        initializeDevice();

      auto OSPRAY_DYNAMIC_LOADBALANCER =
          getEnvVar<int>("OSPRAY_DYNAMIC_LOADBALANCER");

      auto useDynamicLoadBalancer =
          getParam<int>("dynamicLoadBalancer",
                     OSPRAY_DYNAMIC_LOADBALANCER.value_or(false));

      auto OSPRAY_PREALLOCATED_TILES =
          utility::getEnvVar<int>("OSPRAY_PREALLOCATED_TILES");

      auto preAllocatedTiles =
          OSPRAY_PREALLOCATED_TILES.value_or(getParam<int>("preAllocatedTiles",4));

      auto OSPRAY_MPI_OFFLOAD_WRITE_BUFFER_SCALE =
          utility::getEnvVar<float>("OSPRAY_MPI_OFFLOAD_WRITE_BUFFER_SCALE");

      auto writeBufferSize =
          OSPRAY_MPI_OFFLOAD_WRITE_BUFFER_SCALE.value_or(getParam<float>("writeBufferScale", 1.f));

      size_t bufferSize = 1024 * size_t(writeBufferSize * 1024);

      writeStream->flush();
      writeStream = make_unique<networking::BufferedWriteStream>(*mpiFabric, bufferSize);

      work::SetLoadBalancer slbWork(ObjectHandle(),
                                    useDynamicLoadBalancer,
                                    preAllocatedTiles);
      processWork(slbWork);
    }

    OSPFrameBuffer
    MPIOffloadDevice::frameBufferCreate(const vec2i &size,
                                        const OSPFrameBufferFormat mode,
                                        const uint32 channels)
    {
      ObjectHandle handle = allocateHandle();
      work::CreateFrameBuffer work(handle, size, mode, channels);
      processWork(work);
      return (OSPFrameBuffer)(int64)handle;
    }


    /*! map frame buffer */
    const void *MPIOffloadDevice::frameBufferMap(OSPFrameBuffer _fb,
                                                 OSPFrameBufferChannel channel)
    {
      ObjectHandle handle = (const ObjectHandle &)_fb;
      FrameBuffer *fb = (FrameBuffer *)handle.lookup();

      return fb->mapBuffer(channel);
    }

    /*! unmap previously mapped frame buffer */
    void MPIOffloadDevice::frameBufferUnmap(const void *mapped,
                                            OSPFrameBuffer _fb)
    {
      ObjectHandle handle = (const ObjectHandle &)_fb;
      FrameBuffer *fb = (FrameBuffer *)handle.lookup();

      fb->unmap(mapped);
    }

    /*! create a new model */
    OSPModel MPIOffloadDevice::newModel()
    {
      ObjectHandle handle = allocateHandle();
      work::NewModel work("", handle);
      processWork(work);
      return (OSPModel)(int64)handle;
    }

    /*! finalize a newly specified model */
    void MPIOffloadDevice::commit(OSPObject _object)
    {
      const ObjectHandle handle = (const ObjectHandle&)_object;
      work::CommitObject work(handle);
      processWork(work);
    }

    /*! add a new geometry to a model */
    void MPIOffloadDevice::addGeometry(OSPModel _model, OSPGeometry _geometry)
    {
      work::AddGeometry work(_model, _geometry);
      processWork(work);
    }

    /*! add a new volume to a model */
    void MPIOffloadDevice::addVolume(OSPModel _model, OSPVolume _volume)
    {
      work::AddVolume work(_model, _volume);
      processWork(work);
    }

    /*! create a new data buffer */
    OSPData MPIOffloadDevice::newData(size_t nitems, OSPDataType format,
                                      const void *init, int flags)
    {
      ObjectHandle handle = allocateHandle();

      work::NewData work(handle, nitems, format, init, flags);
      processWork(work);

      return (OSPData)(int64)handle;
    }

    /*! assign (named) string parameter to an object */
    void MPIOffloadDevice::setVoidPtr(OSPObject _object,
                                      const char *bufName,
                                      void *v)
    {
      UNUSED(_object, bufName, v);
      throw std::runtime_error("setting a void pointer as parameter to an "
                               "object is not allowed in MPI mode");
    }

    void MPIOffloadDevice::removeParam(OSPObject object, const char *name)
    {
      work::RemoveParam work((ObjectHandle&)object, name);
      processWork(work);
    }

    /*! Copy data into the given object. */
    int MPIOffloadDevice::setRegion(OSPVolume _volume, const void *source,
                                    const vec3i &index, const vec3i &count)
    {
      char *typeString = nullptr;
      getString(_volume, "voxelType", &typeString);
      OSPDataType type = typeForString(typeString);
      delete [] typeString;

      Assert(type != OSP_UNKNOWN && "unknown volume voxel type");
      work::SetRegion work(_volume, index, count, source, type);
      processWork(work);
      return true;
    }

    /*! assign (named) string parameter to an object */
    void MPIOffloadDevice::setString(OSPObject _object,
                                     const char *bufName,
                                     const char *s)
    {
      work::SetParam<std::string> work((ObjectHandle&)_object, bufName, s);
      processWork(work);
    }

    /*! load module */
    int MPIOffloadDevice::loadModule(const char *name)
    {
      work::LoadModule work(name);
      processWork(work, true);
      return work.errorCode;
    }

    /*! assign (named) float parameter to an object */
    void MPIOffloadDevice::setBool(OSPObject _object,
                                   const char *bufName,
                                   const bool b)
    {
      work::SetParam<bool> work((ObjectHandle&)_object, bufName, b);
      processWork(work);
    }

    /*! assign (named) float parameter to an object */
    void MPIOffloadDevice::setFloat(OSPObject _object,
                                    const char *bufName,
                                    const float f)
    {
      work::SetParam<float> work((ObjectHandle&)_object, bufName, f);
      processWork(work);
    }

    /*! assign (named) int parameter to an object */
    void MPIOffloadDevice::setInt(OSPObject _object,
                                  const char *bufName,
                                  const int i)
    {
      work::SetParam<int> work((ObjectHandle&)_object, bufName, i);
      processWork(work);
    }

    /*! assign (named) vec2f parameter to an object */
    void MPIOffloadDevice::setVec2f(OSPObject _object,
                                    const char *bufName,
                                    const vec2f &v)
    {
      work::SetParam<vec2f> work((ObjectHandle&)_object, bufName, v);
      processWork(work);
    }

    /*! assign (named) vec3f parameter to an object */
    void MPIOffloadDevice::setVec3f(OSPObject _object,
                                    const char *bufName,
                                    const vec3f &v)
    {
      work::SetParam<vec3f> work((ObjectHandle&)_object, bufName, v);
      processWork(work);
    }

    /*! assign (named) vec4f parameter to an object */
    void MPIOffloadDevice::setVec4f(OSPObject _object,
                                    const char *bufName,
                                    const vec4f &v)
    {
      work::SetParam<vec4f> work((ObjectHandle&)_object, bufName, v);
      processWork(work);
    }

    /*! assign (named) vec2i parameter to an object */
    void MPIOffloadDevice::setVec2i(OSPObject _object,
                                    const char *bufName,
                                    const vec2i &v)
    {
      work::SetParam<vec2i> work((ObjectHandle&)_object, bufName, v);
      processWork(work);
    }

    /*! assign (named) vec3i parameter to an object */
    void MPIOffloadDevice::setVec3i(OSPObject _object,
                                    const char *bufName,
                                    const vec3i &v)
    {
      work::SetParam<vec3i> work((ObjectHandle&)_object, bufName, v);
      processWork(work);
    }

    /*! assign (named) data item as a parameter to an object */
    void MPIOffloadDevice::setObject(OSPObject _target,
                                     const char *bufName,
                                     OSPObject _value)
    {
      work::SetParam<OSPObject> work((ObjectHandle&)_target, bufName, _value);
      processWork(work);
    }

    /*! create a new pixelOp object (out of list of registered pixelOps) */
    OSPPixelOp MPIOffloadDevice::newPixelOp(const char *type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewPixelOp work(type, handle);
      processWork(work);
      return (OSPPixelOp)(int64)handle;
    }

    /*! set a frame buffer's pixel op object */
    void MPIOffloadDevice::setPixelOp(OSPFrameBuffer _fb, OSPPixelOp _op)
    {
      work::SetPixelOp work(_fb, _op);
      processWork(work);
    }

    /*! create a new renderer object (out of list of registered renderers) */
    OSPRenderer MPIOffloadDevice::newRenderer(const char *type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewRenderer work(type, handle);
      processWork(work);
      return (OSPRenderer)(int64)handle;
    }

    /*! create a new camera object (out of list of registered cameras) */
    OSPCamera MPIOffloadDevice::newCamera(const char *type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewCamera work(type, handle);
      processWork(work);
      return (OSPCamera)(int64)handle;
    }

    /*! create a new volume object (out of list of registered volumes) */
    OSPVolume MPIOffloadDevice::newVolume(const char *type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewVolume work(type, handle);
      processWork(work);
      return (OSPVolume)(int64)handle;
    }

    /*! create a new geometry object (out of list of registered geometries) */
    OSPGeometry MPIOffloadDevice::newGeometry(const char *type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewGeometry work(type, handle);
      processWork(work);
      return (OSPGeometry)(int64)handle;
    }

    /*! have given renderer create a new material */
    OSPMaterial MPIOffloadDevice::newMaterial(OSPRenderer renderer,
                                              const char *material_type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewMaterial work(renderer, material_type, handle);
      processWork(work);
      return (OSPMaterial)(int64)handle;
    }

    /*! have given renderer create a new material */
    OSPMaterial MPIOffloadDevice::newMaterial(const char *renderer_type,
                                              const char *material_type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewMaterial2 work(renderer_type, material_type, handle);
      processWork(work);
      return (OSPMaterial)(int64)handle;
    }

    /*! create a new transfer function object (out of list of
        registered transfer function types) */
    OSPTransferFunction MPIOffloadDevice::newTransferFunction(const char *type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewTransferFunction work(type, handle);
      processWork(work);
      return (OSPTransferFunction)(int64)handle;
    }

    /*! have given renderer create a new Light */
    OSPLight MPIOffloadDevice::newLight(const char *type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewLight work(type, handle);
      processWork(work);
      return (OSPLight)(int64)handle;
    }

    /*! clear the specified channel(s) of the frame buffer specified in
        'whichChannels'

      if whichChannel&OSP_FB_COLOR!=0, clear the color buffer to
      '0,0,0,0'.

      if whichChannel&OSP_FB_DEPTH!=0, clear the depth buffer to
      +inf.

      if whichChannel&OSP_FB_ACCUM!=0, clear the accum buffer to 0,0,0,0,
      and reset accumID.
    */
    void MPIOffloadDevice::frameBufferClear(OSPFrameBuffer _fb,
                                     const uint32 fbChannelFlags)
    {
      work::ClearFrameBuffer work(_fb, fbChannelFlags);
      processWork(work);
    }

    /*! remove an existing geometry from a model */
    void MPIOffloadDevice::removeGeometry(OSPModel _model,
                                          OSPGeometry _geometry)
    {
      work::RemoveGeometry work(_model, _geometry);
      processWork(work);
    }

    /*! remove an existing volume from a model */
    void MPIOffloadDevice::removeVolume(OSPModel _model, OSPVolume _volume)
    {
      work::RemoveVolume work(_model, _volume);
      processWork(work);
    }

    /*! call a renderer to render a frame buffer */
    float MPIOffloadDevice::renderFrame(OSPFrameBuffer _fb,
                                        OSPRenderer _renderer,
                                        const uint32 fbChannelFlags)
    {
      work::RenderFrame work(_fb, _renderer, fbChannelFlags);
      processWork(work, true);
      return work.varianceResult;
    }

    //! release (i.e., reduce refcount of) given object
    /*! note that all objects in ospray are refcounted, so one cannot
      explicitly "delete" any object. instead, each object is created
      with a refcount of 1, and this refcount will be
      increased/decreased every time another object refers to this
      object resp releases its hold on it; if the refcount is 0 the
      object will automatically get deleted. For example, you can
      create a new material, assign it to a geometry, and immediately
      after this assignation release its refcount; the material will
      stay 'alive' as long as the given geometry requires it. */
    void MPIOffloadDevice::release(OSPObject _obj)
    {
      work::CommandRelease work((const ObjectHandle&)_obj);
      processWork(work);
    }

    //! assign given material to given geometry
    void MPIOffloadDevice::setMaterial(OSPGeometry _geometry,
                                       OSPMaterial _material)
    {
      work::SetMaterial work((ObjectHandle&)_geometry, _material);
      processWork(work);
    }

    /*! create a new Texture2D object */
    OSPTexture MPIOffloadDevice::newTexture(const char *type)
    {
      ObjectHandle handle = allocateHandle();
      work::NewTexture work(type, handle);
      processWork(work);
      return (OSPTexture)(int64)handle;
    }

    int MPIOffloadDevice::getString(OSPObject _object,
                                    const char *name,
                                    char **value)
    {
      ManagedObject *object = ((ObjectHandle&)_object).lookup();
      if (object->hasParam(name)) {
        *value = new char[2048];
        strncpy(*value, object->getParam<std::string>(name, "").c_str(), 2048);
        return true;
      }
      return false;
    }

    OSPPickResult MPIOffloadDevice::pick(OSPRenderer renderer,
                                         const vec2f &screenPos)
    {
      work::Pick work(renderer, screenPos);
      processWork(work, true);
      return work.pickResult;
    }

    void MPIOffloadDevice::processWork(work::Work &work, bool flushWriteStream)
    {
      static size_t numWorkSent = 0;
      postStatusMsg(OSPRAY_MPI_VERBOSE_LEVEL)
          << "#osp.mpi.master: processing/sending work item "
          << numWorkSent++;

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

  } // ::ospray::mpi
} // ::ospray

extern "C" OSPRAY_DLLEXPORT void ospray_init_module_mpi()
{
}
