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
#include "mpiCommon/async/CommLayer.h"
#include "mpi/MPIDevice.h"
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
#include "common/OSPWork.h"
// std
#ifndef _WIN32
#  include <unistd.h> // for fork()
#  include <dlfcn.h>
#endif

namespace ospray {
  using std::cout;
  using std::endl;

  namespace mpi {

    //! this runs an ospray worker process.
    /*! it's up to the proper init routine to decide which processes
      call this function and which ones don't. This function will not
      return. */
    OSPRAY_MPI_INTERFACE void runWorker();

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
    void createMPI_runOnExistingRanks(int *ac, const char **av,
                                      bool ranksBecomeWorkers)
    {
      MPI_Status status;
      mpi::init(ac,av);
      printf("#o: initMPI::OSPonRanks: %i/%i\n",world.rank,world.size);
      MPI_Barrier(MPI_COMM_WORLD);
      if (world.size <= 1) {
        throw std::runtime_error("No MPI workers found.\n#osp:mpi: Fatal Error "
                                 "- OSPRay told to run in MPI mode, but there "
                                 "seems to be no MPI peers!?\n#osp:mpi: (Did "
                                 "you forget an 'mpirun' in front of your "
                                 "application?)");
      }

      if (world.rank == 0) {
        // we're the root
        MPI_Comm_split(mpi::world.comm,1,mpi::world.rank,&app.comm);
        app.makeIntraComm();
        // app.makeIntercomm();
        printf("#w: app process %i/%i (global %i/%i)\n",
               app.rank,app.size,world.rank,world.size);

        MPI_Intercomm_create(app.comm, 0, world.comm, 1, 1, &worker.comm);
        // worker.makeIntracomm();
        worker.makeInterComm();

        printf("#m: ping-ponging a test message to every worker...\n");
        for (int i=0;i<worker.size;i++) {
          printf("#m: sending tag %i to worker %i\n",i,i);
          MPI_Send(&i,1,MPI_INT,i,i,worker.comm);
          int reply;
          MPI_Recv(&reply,1,MPI_INT,i,i,worker.comm,&status);
          Assert(reply == i);
        }

        MPI_Barrier(MPI_COMM_WORLD);
        // -------------------------------------------------------
        // at this point, all processes should be set up and synced. in
        // particular:
        // - app has intracommunicator to all workers (and vica versa)
        // - app process(es) are in one intercomm ("app"); workers all in
        //   another ("worker")
        // - all processes (incl app) have barrier'ed, and thus now in sync.
      } else {
        // we're the workers
        MPI_Comm_split(mpi::world.comm,0,mpi::world.rank,&worker.comm);
        worker.makeIntraComm();
        // worker.makeIntercomm();
        printf("#w: worker process %i/%i (global %i/%i)\n",
               worker.rank,worker.size,world.rank,world.size);

        MPI_Intercomm_create(worker.comm, 0, world.comm, 0, 1, &app.comm);
        app.makeInterComm();
        // app.makeIntracomm();
        // worker.containsMe = true;
        // app.containsMe = false;

        // replying to test-message
        printf("#w: worker %i trying to receive tag %i...\n",
               worker.rank,worker.rank);
        int reply;
        MPI_Recv(&reply,1,MPI_INT,0,worker.rank,app.comm,&status);
        MPI_Send(&reply,1,MPI_INT,0,worker.rank,app.comm);

        MPI_Barrier(MPI_COMM_WORLD);
        // -------------------------------------------------------
        // at this point, all processes should be set up and synced. in
        // particular:
        // - app has intracommunicator to all workers (and vica versa)
        // - app process(es) are in one intercomm ("app"); workers all in
        //   another ("worker")
        // - all processes (incl app) have barrier'ed, and thus now in sync.

        // now, all workers will enter their worker loop (ie, they will *not*
        // return)
        if (ranksBecomeWorkers) {
          cout << "RUNNING WORKER W/O RETURNING!" << endl;
          mpi::runWorker();
          throw std::runtime_error("should never reach here!");
          /* no return here - 'runWorker' will never return */
        } else {
          cout << "#osp:mpi: distributed mode detected, "
               << "returning device on all ranks!" << endl;
        }
      }
    }

    void createMPI_RanksBecomeWorkers(int *ac, const char **av)
    {
      createMPI_runOnExistingRanks(ac,av,true);
    }

    /*! in this mode ("separate worker group" mode)
      - the user may or may not have launched MPI explicitly for his app
      - the app may or may not be running distributed
      - the ospray frontend (the part linked to the app) will wait for a remote
        MPI gruop of
      workers to connect to this app
      - the ospray frontend will store the port its waiting on connections for
        in the
      filename passed to this function; or will print it to stdout if this is
      nullptr
    */
    void createMPI_ListenForWorkers(int *ac, const char **av,
                                    const char *fileNameToStorePortIn)
    {
      mpi::init(ac,av);

      if (world.rank < 1) {
        cout << "======================================================="
             << endl;
        cout << "initializing OSPRay MPI in 'listening for workers' mode"
             << endl;
        cout << "======================================================="
             << endl;
      }
      int rc;

      app.comm = world.comm;
      app.makeIntraComm();
      // app.makeIntercomm();

      char appPortName[MPI_MAX_PORT_NAME];
      if (world.rank == 0) {
        rc = MPI_Open_port(MPI_INFO_NULL, appPortName);
        Assert(rc == MPI_SUCCESS);

        // fix port name: replace all '$'s by '%'s to allow using it on the
        //                cmdline...
        char *fixedPortName = strdup(appPortName);
        for (char *s = fixedPortName; *s; ++s)
          if (*s == '$') *s = '%';

        cout << "#a: ospray app started, waiting for service connect"  << endl;
        cout << "#a: at port " << fixedPortName << endl;

        if (fileNameToStorePortIn) {
          FILE *file = fopen(fileNameToStorePortIn,"w");
          Assert2(file,"could not open file to store listen port in");
          fprintf(file,"%s",fixedPortName);
          fclose(file);
          cout << "#a: (ospray port name store in file '"
               << fileNameToStorePortIn << "')" << endl;
        }
      }
      rc = MPI_Comm_accept(appPortName,MPI_INFO_NULL,0,app.comm,&worker.comm);
      Assert(rc == MPI_SUCCESS);
      worker.makeInterComm();
      // worker.makeIntracomm();

      if (app.rank == 0) {
        cout << "======================================================="
             << endl;
        cout << "OSPRAY Worker ring connected" << endl;
        cout << "======================================================="
             << endl;
      }
      MPI_Barrier(app.comm);

      if (app.rank == 1) {
        cout << "WARNING: you're trying to run an mpi-parallel app with ospray;"
             << endl
             << " only the root rank is allowed to issue ospray calls" << endl;
        cout << "======================================================="
             << endl;
      }

      MPI_Barrier(app.comm);
    }

    /*! in this mode ("separate worker group" mode)
      - the user may or may not have launched MPI explicitly for his app
      - the app may or may not be running distributed
      - the ospray frontend (the part linked to the app) will use the specified
      'launchCmd' to launch a _new_ MPI group of worker processes.
      - the ospray frontend will assume the launched process to output 'OSPRAY_SERVICE_PORT'
      stdout, will parse that output for this string, and create an mpi connection to
      this port to establish the service
    */
    void createMPI_LaunchWorkerGroup(int *ac, const char **av,
                                     const char *launchCommand)
    {
      int rc;

      mpi::init(ac,av);

      Assert(launchCommand);

      MPI_Comm_dup(world.comm,&app.comm);
      app.makeIntraComm();
      // app.makeIntercomm();

      char appPortName[MPI_MAX_PORT_NAME];
      if (app.rank == 0 || app.size == -1) {
        cout << "======================================================="
             << endl;
        cout << "initializing OSPRay MPI in 'launching new workers' mode"
             << endl;
        cout << "using launch script '" << launchCommand << "'" << endl;
        rc = MPI_Open_port(MPI_INFO_NULL, appPortName);
        Assert(rc == MPI_SUCCESS);

        // fix port name: replace all '$'s by '%'s to allow using it on the
        //                cmdline...
        char *fixedPortName = strdup(appPortName);
        cout << "with port " << fixedPortName <<  endl;
        cout << "======================================================="
             << endl;
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
          cout << "OSPRay worker process has died - killing application"
               << endl;
          exit(0);
        }
#endif
      }

      rc = MPI_Comm_accept(appPortName,MPI_INFO_NULL,0,app.comm,&worker.comm);
      Assert(rc == MPI_SUCCESS);
      worker.makeInterComm();
      // worker.makeIntracomm();
      if (app.rank == 0 || app.size == -1) {
        cout << "OSPRay MPI Worker ring successfully connected." << endl;
        cout << "found " << worker.size << " workers." << endl;
        cout << "======================================================="
             << endl;
      }
      if (app.size > 1) {
        if (app.rank == 1) {
          cout << "ospray:WARNING: you're trying to run an mpi-parallel app "
               << "with ospray\n"
               << "(only the root node is allowed to issue ospray api "
               << "calls right now)\n";
          cout << "======================================================="
               << endl;
        }

        MPI_Barrier(app.comm);
      }

      MPI_Barrier(app.comm);
    }
    
    MPIDevice::MPIDevice() : bufferedComm(std::make_shared<BufferedMPIComm>()){}

    MPIDevice::~MPIDevice()
    {
      // NOTE(jda) - Seems that there are no MPI Devices on worker ranks...this
      //             will likely change for data-distributed API settings?
      if (mpi::world.rank == 0) {
        std::cout << "shutting down mpi device" << std::endl;
        work::CommandFinalize work;
        processWork(&work);
      }
    }

    void MPIDevice::commit()
    {
      if (initialized)// NOTE (jda) - doesn't make sense to commit again?
        return;

      Device::commit();

      initialized = true;

      int _ac = 2;
      const char *_av[] = {"ospray_mpi_worker", "--osp:mpi"};

      std::string mode = getParamString("mpiMode", "mpi");

      if (mode == "mpi") {
        createMPI_RanksBecomeWorkers(&_ac,_av);
      }
      else if(mode == "mpi-launch") {
        std::string launchCommand = getParamString("launchCommand", "");

        if (launchCommand.empty()) {
          throw std::runtime_error("You must provide the launchCommand "
                                   "parameter in mpi-launch mode!");
        }

        createMPI_LaunchWorkerGroup(&_ac,_av,launchCommand.c_str());
      }
      else if (mode == "mpi-listen") {
        std::string fileNameToStorePortIn =
            getParamString("fileNameToStorePortIn", "");

        if (fileNameToStorePortIn.empty()) {
          throw std::runtime_error("You must provide the fileNameToStorePortIn "
                                   "parameter in mpi-listen mode!");
        }

        createMPI_ListenForWorkers(&_ac,_av,fileNameToStorePortIn.c_str());
      }
      else {
        throw std::runtime_error("Invalid MPI mode!");
      }

      TiledLoadBalancer::instance = make_unique<staticLoadBalancer::Master>();

      if (mpi::world.size != 1) {
        if (mpi::world.rank < 0) {
          PRINT(mpi::world.rank);
          PRINT(mpi::world.size);
          throw std::runtime_error("OSPRay MPI startup error. Use \"mpirun "
                                   "-n 1 <command>\" when calling an "
                                   "application that tries to spawn to start "
                                   "the application you were trying to "
                                   "start.");
        }
      }
    }

    OSPFrameBuffer 
    MPIDevice::frameBufferCreate(const vec2i &size, 
                                 const OSPFrameBufferFormat mode,
                                 const uint32 channels)
    {
      ObjectHandle handle = allocateHandle();
      work::CreateFrameBuffer work(handle, size, mode, channels);
      processWork(&work);
      return (OSPFrameBuffer)(int64)handle;
    }


    /*! map frame buffer */
    const void *MPIDevice::frameBufferMap(OSPFrameBuffer _fb,
                                          OSPFrameBufferChannel channel)
    {
      ObjectHandle handle = (const ObjectHandle &)_fb;
      FrameBuffer *fb = (FrameBuffer *)handle.lookup();

      switch (channel) {
      case OSP_FB_COLOR: return fb->mapColorBuffer();
      case OSP_FB_DEPTH: return fb->mapDepthBuffer();
      default: return nullptr;
      }
    }

    /*! unmap previously mapped frame buffer */
    void MPIDevice::frameBufferUnmap(const void *mapped,
                                     OSPFrameBuffer _fb)
    {
      ObjectHandle handle = (const ObjectHandle &)_fb;
      FrameBuffer *fb = (FrameBuffer *)handle.lookup();
      fb->unmap(mapped);
    }

    /*! create a new model */
    OSPModel MPIDevice::newModel()
    {
      ObjectHandle handle = allocateHandle();
      work::NewObject<Model> work("", handle);
      processWork(&work);
      return (OSPModel)(int64)handle;
    }

    /*! finalize a newly specified model */
    void MPIDevice::commit(OSPObject _object)
    {
      Assert(_object);
      const ObjectHandle handle = (const ObjectHandle&)_object;
      work::CommitObject work(handle);
      processWork(&work);
    }

    /*! add a new geometry to a model */
    void MPIDevice::addGeometry(OSPModel _model, OSPGeometry _geometry)
    {
      Assert(_model);
      Assert(_geometry);
      work::AddObject<OSPGeometry> work(_model, _geometry);
      processWork(&work);
    }

    /*! add a new volume to a model */
    void MPIDevice::addVolume(OSPModel _model, OSPVolume _volume)
    {
      Assert(_model);
      Assert(_volume);
      work::AddObject<OSPVolume> work(_model, _volume);
      processWork(&work);
    }

    /*! create a new data buffer */
    OSPData MPIDevice::newData(size_t nitems, OSPDataType format,
                               void *init, int flags)
    {
      ObjectHandle handle = allocateHandle();
      // If we're in mastered mode you can't share data with remote nodes
      if (currentApiMode == OSPD_MODE_MASTERED) {
        flags = flags & ~OSP_DATA_SHARED_BUFFER;
      }
      work::NewData work(handle, nitems, format, init, flags);
      processWork(&work);
      return (OSPData)(int64)handle;
    }

    /*! assign (named) string parameter to an object */
    void MPIDevice::setVoidPtr(OSPObject _object, const char *bufName, void *v)
    {
      UNUSED(_object, bufName, v);
      throw std::runtime_error("setting a void pointer as parameter to an "
                               "object is not allowed in MPI mode");
    }

    void MPIDevice::removeParam(OSPObject object, const char *name)
    {
      Assert(object != nullptr  && "invalid object handle");
      Assert(name != nullptr && "invalid identifier for object parameter");
      work::RemoveParam work((ObjectHandle&)object, name);
      processWork(&work);
    }

    /*! Copy data into the given object. */
    int MPIDevice::setRegion(OSPVolume _volume, const void *source,
                             const vec3i &index, const vec3i &count)
    {
      Assert(_volume);
      Assert(source);

      char *typeString = nullptr;
      getString(_volume, "voxelType", &typeString);
      OSPDataType type = typeForString(typeString);
      delete[] typeString;

      Assert(type != OSP_UNKNOWN && "unknown volume voxel type");
      // TODO: should we be counting and reporting failures of setRegion
      // like before?
      work::SetRegion work(_volume, index, count, source, type);
      processWork(&work);
      return true;
    }

    /*! assign (named) string parameter to an object */
    void MPIDevice::setString(OSPObject _object,
                              const char *bufName,
                              const char *s)
    {
      Assert(_object);
      Assert(bufName);
      work::SetParam<std::string> work((ObjectHandle&)_object, bufName, s);
      processWork(&work);
    }

    /*! load module */
    int MPIDevice::loadModule(const char *name)
    {
      work::LoadModule work(name);
      processWork(&work);
      // FIXME: actually we should return an error code here...
      // TODO: If some fail? Can we assume if the master succeeds in loading
      // that all have succeeded in loading? I don't think so.
      return 0;
    }

    /*! assign (named) float parameter to an object */
    void MPIDevice::setFloat(OSPObject _object,
                             const char *bufName,
                             const float f)
    {
      Assert(_object);
      Assert(bufName);
      work::SetParam<float> work((ObjectHandle&)_object, bufName, f);
      processWork(&work);
    }

    /*! assign (named) int parameter to an object */
    void MPIDevice::setInt(OSPObject _object, const char *bufName, const int i)
    {
      Assert(_object);
      Assert(bufName);
      work::SetParam<int> work((ObjectHandle&)_object, bufName, i);
      processWork(&work);
    }

    /*! assign (named) vec2f parameter to an object */
    void MPIDevice::setVec2f(OSPObject _object,
                             const char *bufName,
                             const vec2f &v)
    {
      Assert(_object);
      Assert(bufName);
      work::SetParam<vec2f> work((ObjectHandle&)_object, bufName, v);
      processWork(&work);
    }

    /*! assign (named) vec3f parameter to an object */
    void MPIDevice::setVec3f(OSPObject _object,
                             const char *bufName,
                             const vec3f &v)
    {
      Assert(_object);
      Assert(bufName);
      work::SetParam<vec3f> work((ObjectHandle&)_object, bufName, v);
      processWork(&work);
    }

    /*! assign (named) vec4f parameter to an object */
    void MPIDevice::setVec4f(OSPObject _object,
                             const char *bufName,
                             const vec4f &v)
    {
      Assert(_object);
      Assert(bufName);
      work::SetParam<vec4f> work((ObjectHandle&)_object, bufName, v);
      processWork(&work);
    }

    /*! assign (named) vec2i parameter to an object */
    void MPIDevice::setVec2i(OSPObject _object,
                             const char *bufName,
                             const vec2i &v)
    {
      Assert(_object);
      Assert(bufName);
      work::SetParam<vec2i> work((ObjectHandle&)_object, bufName, v);
      processWork(&work);
    }

    /*! assign (named) vec3i parameter to an object */
    void MPIDevice::setVec3i(OSPObject _object,
                             const char *bufName,
                             const vec3i &v)
    {
      Assert(_object);
      Assert(bufName);
      work::SetParam<vec3i> work((ObjectHandle&)_object, bufName, v);
      processWork(&work);
    }

    /*! assign (named) data item as a parameter to an object */
    void MPIDevice::setObject(OSPObject _target,
                              const char *bufName,
                              OSPObject _value)
    {
      Assert(_target != nullptr);
      Assert(bufName != nullptr);
      work::SetParam<OSPObject> work((ObjectHandle&)_target, bufName, _value);
      processWork(&work);
    }

    /*! create a new pixelOp object (out of list of registered pixelOps) */
    OSPPixelOp MPIDevice::newPixelOp(const char *type)
    {
      Assert(type != nullptr);

      ObjectHandle handle = allocateHandle();
      work::NewObject<PixelOp> work(type, handle);
      processWork(&work);
      return (OSPPixelOp)(int64)handle;
    }

    /*! set a frame buffer's pixel op object */
    void MPIDevice::setPixelOp(OSPFrameBuffer _fb, OSPPixelOp _op)
    {
      Assert(_fb != nullptr);
      Assert(_op != nullptr);
      work::SetPixelOp work(_fb, _op);
      processWork(&work);
    }

    /*! create a new renderer object (out of list of registered renderers) */
    OSPRenderer MPIDevice::newRenderer(const char *type)
    {
      Assert(type != nullptr);

      ObjectHandle handle = allocateHandle();
      work::NewObject<Renderer> work(type, handle);
      processWork(&work);
      return (OSPRenderer)(int64)handle;
    }

    /*! create a new camera object (out of list of registered cameras) */
    OSPCamera MPIDevice::newCamera(const char *type)
    {
      Assert(type != nullptr);
      ObjectHandle handle = allocateHandle();
      work::NewObject<Camera> work(type, handle);
      processWork(&work);
      return (OSPCamera)(int64)handle;
    }

    /*! create a new volume object (out of list of registered volumes) */
    OSPVolume MPIDevice::newVolume(const char *type)
    {
      Assert(type != nullptr);

      ObjectHandle handle = allocateHandle();
      work::NewObject<Volume> work(type, handle);
      processWork(&work);
      return (OSPVolume)(int64)handle;
    }

    /*! create a new geometry object (out of list of registered geometries) */
    OSPGeometry MPIDevice::newGeometry(const char *type)
    {
      Assert(type != nullptr);

      ObjectHandle handle = allocateHandle();
      work::NewObject<Geometry> work(type, handle);
      processWork(&work);
      return (OSPGeometry)(int64)handle;
    }

    /*! have given renderer create a new material */
    OSPMaterial MPIDevice::newMaterial(OSPRenderer _renderer, const char *type)
    {
      if (type == nullptr)
        throw std::runtime_error("#osp:mpi:newMaterial: NULL material type");
      
      if (_renderer == nullptr)
        throw std::runtime_error("#osp:mpi:newMaterial: NULL renderer handle");

      ObjectHandle handle = allocateHandle();
      work::NewRendererObject<Material> work(type, _renderer, handle);
      processWork(&work);
      // TODO: Should we be tracking number of failures? Shouldn't they
      // all fail or not fail?
      return (OSPMaterial)(int64)handle;
    }

      /*! create a new transfer function object (out of list of
        registered transfer function types) */
    OSPTransferFunction MPIDevice::newTransferFunction(const char *type)
    {
      Assert(type != nullptr);

      ObjectHandle handle = allocateHandle();
      work::NewObject<TransferFunction> work(type, handle);
      processWork(&work);
      return (OSPTransferFunction)(int64)handle;
    }


    /*! have given renderer create a new Light */
    OSPLight MPIDevice::newLight(OSPRenderer _renderer, const char *type)
    {
      if (type == nullptr)
        throw std::runtime_error("#osp:mpi:newLight: NULL light type");

      ObjectHandle handle = allocateHandle();
      work::NewRendererObject<Light> work(type, _renderer, handle);
      processWork(&work);
      // TODO: Should we be tracking number of failures? Shouldn't they
      // all fail or not fail?
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
    void MPIDevice::frameBufferClear(OSPFrameBuffer _fb,
                                     const uint32 fbChannelFlags)
    {
      work::ClearFrameBuffer work(_fb, fbChannelFlags);
      processWork(&work);
    }

    /*! remove an existing geometry from a model */
    void MPIDevice::removeGeometry(OSPModel _model, OSPGeometry _geometry)
    {
      work::RemoveObject<OSPGeometry> work(_model, _geometry);
      processWork(&work);
    }

    /*! remove an existing volume from a model */
    void MPIDevice::removeVolume(OSPModel _model, OSPVolume _volume)
    {
      work::RemoveObject<OSPVolume> work(_model, _volume);
      processWork(&work);
    }


    /*! call a renderer to render a frame buffer */
    float MPIDevice::renderFrame(OSPFrameBuffer _fb,
                                OSPRenderer _renderer,
                                const uint32 fbChannelFlags)
    {
      // Note: render frame is flushing so the work error result will be set,
      // since the master participates in rendering
      work::RenderFrame work(_fb, _renderer, fbChannelFlags);
      processWork(&work);
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
    void MPIDevice::release(OSPObject _obj)
    {
      if (!_obj) return;
      work::CommandRelease work((const ObjectHandle&)_obj);
      processWork(&work);
    }

    //! assign given material to given geometry
    void MPIDevice::setMaterial(OSPGeometry _geometry, OSPMaterial _material)
    {
      work::SetParam<OSPMaterial> work((ObjectHandle&)_geometry, _material);
      processWork(&work);
    }

    /*! create a new Texture2D object */
    OSPTexture2D MPIDevice::newTexture2D(const vec2i &sz,
        const OSPTextureFormat type, void *data, const uint32 flags)
    {
      ObjectHandle handle = allocateHandle();
      work::NewTexture2d work(handle, sz, type, data, flags);
      processWork(&work);
      return (OSPTexture2D)(int64)handle;
    }

    /*! return a string represenging the given API Mode */
    const char *apiModeName(int mode)
    {
      switch (mode) {
      case OSPD_MODE_INDEPENDENT:
        return "OSPD_MODE_INDEPENDENT";
      case OSPD_MODE_MASTERED:
        return "OSPD_MODE_MASTERED";
      case OSPD_MODE_COLLABORATIVE:
        return "OSPD_MODE_COLLABORATIVE";
      default:
        PRINT(mode);
        NOTIMPLEMENTED;
      };
    }

    /*! switch API mode for distriubted API extensions */
    void MPIDevice::apiMode(OSPDApiMode newMode)
    {
      printf("rank %i asked to go from %s mode to %s mode\n",
             mpi::world.rank,apiModeName(currentApiMode),apiModeName(newMode));
      switch (currentApiMode) {
        // ==================================================================
        // ==================================================================
      case OSPD_MODE_INDEPENDENT: {
        switch (newMode) {
        case OSPD_MODE_COLLABORATIVE:
        case OSPD_MODE_INDEPENDENT:
          currentApiMode = newMode;
          // It's probably worth making this an explicit sync point
          // between app/worker ranks. TODO: What comm to barrier on?
          // MPI_Barrier(MPI_COMM_WORLD);
          mpi::barrier(mpi::world);
          break;
        case OSPD_MODE_MASTERED:
          NOTIMPLEMENTED;
        }
      } break;
        // ==================================================================
        // currently in default (mastered) mode where master tells workers what
        // to do
        // ==================================================================
      case OSPD_MODE_MASTERED: {
        // first: tell workers to switch to new mode: they're in
        // mastered mode and thus waiting for *us* to tell them what
        // to do, so let's do it.
        switch (newMode) {
        case OSPD_MODE_MASTERED: {
          // nothing to do, actually, the workers are already in this
          // mode, no use sending this request again
          printf("rank %i remaining in mastered mode\n",mpi::world.rank);
        } break;
        case OSPD_MODE_INDEPENDENT:
        case OSPD_MODE_COLLABORATIVE: {
          printf("rank %i telling clients to switch to %s mode.\n",
                 mpi::world.rank,apiModeName(newMode));
          // cmd.newCommand(CMD_API_MODE);
          // cmd.send((int32)newMode);
          // cmd.flush();
          currentApiMode = newMode;
          // and just to be sure, do a barrier here -- not acutally needed
          // AFAICT.
          mpi::barrier(mpi::world);
          // MPI_Barrier(MPI_COMM_WORLD);
        } break;
        default:
          NOTIMPLEMENTED;
        };
      } break;
        // ==================================================================
        // ==================================================================
      case OSPD_MODE_COLLABORATIVE: {
        switch (newMode) {
        case OSPD_MODE_COLLABORATIVE:
        case OSPD_MODE_INDEPENDENT:
          currentApiMode = newMode;
          // It's probably worth making this an explicit sync point
          // between app/worker ranks. TODO: What comm to barrier on?
          mpi::barrier(mpi::world);
          // MPI_Barrier(MPI_COMM_WORLD);
          break;
        case OSPD_MODE_MASTERED:
          NOTIMPLEMENTED;
        }
      } break;

        // ==================================================================
        // this mode should not exit - implementation error
        // ==================================================================
      default:
        NOTIMPLEMENTED;
      };
    }

    void MPIDevice::sampleVolume(float **results,
                                 OSPVolume volume,
                                 const vec3f *worldCoordinates,
                                 const size_t &count)
    {
      Assert2(volume, "invalid volume handle");
      Assert2(worldCoordinates, "invalid worldCoordinates");

      Assert2(0, "not implemented");
    }

    int MPIDevice::getString(OSPObject _object, const char *name, char **value)
    {
      Assert(_object);
      Assert(name);

      ManagedObject *object = ((ObjectHandle&)_object).lookup();
      ManagedObject::Param *param = object->findParam(name);
      bool foundParameter = (param != NULL && param->type == OSP_STRING);
      if (foundParameter) {
        *value = new char[2048];
        strncpy(*value, param->s->c_str(), 2048);
        return true;
      }
      return false;
    }

    void MPIDevice::processWork(work::Work* work)
    {
      if (currentApiMode == OSPD_MODE_MASTERED) {
        bufferedComm->send(mpi::Address(&mpi::worker,(int32)mpi::SEND_ALL), work);
        // TODO: Maybe instead of this we can have a concept of "flushing" work units
        if (work->flushing()) {
          bufferedComm->flush();
        }
        // Run the master side variant of the work unit
        work->runOnMaster();
      } else {
        work->run();
      }
    }

    ObjectHandle MPIDevice::allocateHandle() const {
      ObjectHandle handle = nullHandle;
      switch (currentApiMode) {
        case OSPD_MODE_MASTERED:
          handle = ObjectHandle::alloc();
          break;
        default:
          throw std::runtime_error("MPIDevice::processWork: Unimplemented mode!");
      }
      return handle;
    }

    OSP_REGISTER_DEVICE(MPIDevice, mpi_device);
    OSP_REGISTER_DEVICE(MPIDevice, mpi);

  } // ::ospray::mpi
} // ::ospray

extern "C" OSPRAY_DLLEXPORT void ospray_init_module_mpi()
{
  ospray::mpi::work::initWorkMap();
  std::cout << "#mpi: initializing ospray MPI plugin" << std::endl;
}

