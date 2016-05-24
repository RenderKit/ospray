// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "ospray/mpi/MPICommon.h"
#include "ospray/mpi/MPIDevice.h"
#include "ospray/common/Model.h"
#include "ospray/common/Data.h"
#include "ospray/common/Library.h"
#include "ospray/geometry/TriangleMesh.h"
#include "ospray/render/Renderer.h"
#include "ospray/camera/Camera.h"
#include "ospray/volume/Volume.h"
#include "ospray/mpi/MPILoadBalancer.h"
#include "ospray/fb/LocalFB.h"
#include "ospray/mpi/async/CommLayer.h"
#include "ospray/mpi/DistributedFrameBuffer.h"
// std
#ifndef _WIN32
#  include <unistd.h> // for fork()
#endif


namespace ospray {
  using std::cout;
  using std::endl;

  namespace mpi {
    //! this runs an ospray worker process. 
    /*! it's up to the proper init routine to decide which processes
      call this function and which ones don't. This function will not
      return. */
    void runWorker(int *_ac, const char **_av);


    /*! in this mode ("ospray on ranks" mode, or "ranks" mode), the
        user has launched the app across all ranks using mpirun "<all
        rank> <app>"; no new processes need to get launched.

        Based on the 'startworkers' flag, this function can set up ospray in one of two modes:
        
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
    ospray::api::Device *createMPI_runOnExistingRanks(int *ac, const char **av, 
                                                      bool ranksBecomeWorkers)
    {
      MPI_Status status;
      mpi::init(ac,av);
      printf("#o: initMPI::OSPonRanks: %i/%i\n",world.rank,world.size);

      MPI_Barrier(MPI_COMM_WORLD);

      if (world.size <= 1) {
        throw std::runtime_error("No MPI workers found.\n#osp:mpi: Fatal Error - OSPRay told to run in MPI mode, but there seems to be no MPI peers!?\n#osp:mpi: (Did you forget an 'mpirun' in front of your application?)");
      }

      if (world.rank == 0) {
#if 0
        ospray::Task::initTaskSystem(debugMode ? 0 : numThreads);
#endif

        // we're the root
        MPI_Comm_split(mpi::world.comm,1,mpi::world.rank,&app.comm);
        app.makeIntraComm();
        // app.makeIntercomm();
        printf("#w: app process %i/%i (global %i/%i)\n",app.rank,app.size,world.rank,world.size);

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
        // at this point, all processes should be set up and synced. in particular
        // - app has intracommunicator to all workers (and vica versa)
        // - app process(es) are in one intercomm ("app"); workers all in another ("worker")
        // - all processes (incl app) have barrier'ed, and thus now in sync.

        // now, root proc(s) will return, initialize the MPI device, then return to the app
        return new mpi::MPIDevice(ac,av);
      } else {
        // we're the workers
        MPI_Comm_split(mpi::world.comm,0,mpi::world.rank,&worker.comm);
        worker.makeIntraComm();
        // worker.makeIntercomm();
        printf("#w: worker process %i/%i (global %i/%i)\n",worker.rank,worker.size,world.rank,world.size);

        MPI_Intercomm_create(worker.comm, 0, world.comm, 0, 1, &app.comm); 
        app.makeInterComm();
        // app.makeIntracomm();
        // worker.containsMe = true;
        // app.containsMe = false;

        // replying to test-message
        printf("#w: worker %i trying to receive tag %i...\n",worker.rank,worker.rank);
        int reply;
        MPI_Recv(&reply,1,MPI_INT,0,worker.rank,app.comm,&status);
        MPI_Send(&reply,1,MPI_INT,0,worker.rank,app.comm);

        MPI_Barrier(MPI_COMM_WORLD);
        // -------------------------------------------------------
        // at this point, all processes should be set up and synced. in particular
        // - app has intracommunicator to all workers (and vica versa)
        // - app process(es) are in one intercomm ("app"); workers all in another ("worker")
        // - all processes (incl app) have barrier'ed, and thus now in sync.

        // now, all workers will enter their worker loop (ie, they will *not* return
        if (ranksBecomeWorkers) {
          cout << "RUNNING WORKER W/O RETURNING!" << endl;
          mpi::runWorker(ac,av);
          throw std::runtime_error("should never reach here!");
          /* no return here - 'runWorker' will never return */
        } else {
          cout << "#osp:mpi: distributed mode detected, returning device on all ranks!" << endl << std::flush;
          return new mpi::MPIDevice(ac,av);
        }
      }
      // nobody should ever come here ...
      return NULL;
    }


    /*! in this mode ("separate worker group" mode)
      - the user may or may not have launched MPI explicitly for his app
      - the app may or may not be running distributed
      - the ospray frontend (the part linked to the app) will wait for a remote MPI gruop of 
      workers to connect to this app
      - the ospray frontend will store the port its waiting on connections for in the
      filename passed to this function; or will print it to stdout if this is NULL
    */
    ospray::api::Device *createMPI_ListenForWorkers(int *ac, const char **av, 
                                                    const char *fileNameToStorePortIn)
    {
      ospray::init(ac,&av);
      mpi::init(ac,av);

      if (world.rank < 1) {
        cout << "=======================================================" << endl;
        cout << "initializing OSPRay MPI in 'listening for workers' mode" << endl;
        cout << "=======================================================" << endl;
      }
      int rc;

      app.comm = world.comm; 
      app.makeIntraComm();
      // app.makeIntercomm();
      
      char appPortName[MPI_MAX_PORT_NAME];
      if (world.rank == 0) {
        rc = MPI_Open_port(MPI_INFO_NULL, appPortName);
        Assert(rc == MPI_SUCCESS);
        
        // fix port name: replace all '$'s by '%'s to allow using it on the cmdline...
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
        cout << "=======================================================" << endl;
        cout << "OSPRAY Worker ring connected" << endl;
        cout << "=======================================================" << endl;
      }
      MPI_Barrier(app.comm);
      
      if (app.rank == 1) {
        cout << "WARNING: you're trying to run an mpi-parallel app with ospray; " << endl
             << " only the root rank is allowed to issue ospray calls" << endl;
        cout << "=======================================================" << endl;
      }
      MPI_Barrier(app.comm);

      if (app.rank >= 1)
        return NULL;
      return new mpi::MPIDevice(ac,av);
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
    ospray::api::Device *createMPI_LaunchWorkerGroup(int *ac, const char **av, 
                                                     const char *launchCommand)
    {
      int rc;

      ospray::init(ac,&av);
      mpi::init(ac,av);

      Assert(launchCommand);

      MPI_Comm_dup(world.comm,&app.comm);
      app.makeIntraComm();
      // app.makeIntercomm();
      
      char appPortName[MPI_MAX_PORT_NAME];
      if (app.rank == 0 || app.size == -1) {
        cout << "=======================================================" << endl;
        cout << "initializing OSPRay MPI in 'launching new workers' mode" << endl;
        cout << "using launch script '" << launchCommand << "'" << endl;
        rc = MPI_Open_port(MPI_INFO_NULL, appPortName);
        Assert(rc == MPI_SUCCESS);
        
        // fix port name: replace all '$'s by '%'s to allow using it on the cmdline...
        char *fixedPortName = strdup(appPortName);
        cout << "with port " << fixedPortName <<  endl;
        cout << "=======================================================" << endl;
        for (char *s = fixedPortName; *s; ++s)
          if (*s == '$') *s = '%';

        char systemCommand[10000];
        sprintf(systemCommand,"%s %s",launchCommand,fixedPortName);
#ifdef _WIN32
        throw std::runtime_error("OSPRay MPI: no fork() yet on Windows");
#else
        if (fork()) {
          system(systemCommand);
          cout << "OSPRay worker process has died - killing application" << endl;
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
        cout << "=======================================================" << endl;
      }
      if (app.size > 1) {
        if (app.rank == 1) {
          cout << "ospray:WARNING: you're trying to run an mpi-parallel app with ospray\n"
               << "(only the root node is allowed to issue ospray api calls right now)\n";
          cout << "=======================================================" << endl;
        }
        MPI_Barrier(app.comm);
        return NULL;
      }
      MPI_Barrier(app.comm);
      return new mpi::MPIDevice(ac,av);
    }

    void initDistributedAPI(int *ac, char ***av, OSPDRenderMode mpiMode)
    {
      int initialized = false;
      MPI_CALL(Initialized(&initialized));
      if (initialized) 
        throw std::runtime_error("OSPRay MPI Error: MPI already Initialized "
                                 "when calling ospMpiInit()");
      
      ospray::mpi::init(ac,(const char **)*av);
      if (mpi::world.size < 2) {
        throw std::runtime_error("#osp:mpi: trying to run distributed API mode "
                                 "with a single rank? (did you forget the "
                                 "'mpirun'?)");
      }

      ospray::api::Device::current
        = ospray::mpi::createMPI_runOnExistingRanks(ac,(const char**)*av,false);
    }
    
    MPIDevice::MPIDevice(// AppMode appMode, OSPMode ospMode,
                         int *_ac, const char **_av)
      : currentApiMode(OSPD_MODE_MASTERED)
    {
      char *logLevelFromEnv = getenv("OSPRAY_LOG_LEVEL");
      if (logLevelFromEnv && logLevel == 0)
        logLevel = atoi(logLevelFromEnv);

      ospray::init(_ac,&_av);

      if (mpi::world.size !=1) {
        if (mpi::world.rank < 0) {
          PRINT(mpi::world.rank);
          PRINT(mpi::world.size);
          throw std::runtime_error("OSPRay MPI startup error. Use \"mpirun "
                                   "-n 1 <command>\" when calling an "
                                   "application that tries to spawn to start "
                                   "the application you were trying to start.");
        }
      }

      TiledLoadBalancer::instance = new mpi::staticLoadBalancer::Master;
    }


    OSPFrameBuffer 
    MPIDevice::frameBufferCreate(const vec2i &size, 
                                 const OSPFrameBufferFormat mode,
                                 const uint32 channels)
    {
      FrameBuffer::ColorBufferFormat colorBufferFormat = mode; //FrameBuffer::RGBA_UINT8;//FLOAT32;
      bool hasDepthBuffer = (channels & OSP_FB_DEPTH)!=0;
      bool hasAccumBuffer = (channels & OSP_FB_ACCUM)!=0;

      ObjectHandle handle = ObjectHandle::alloc();
      
      FrameBuffer *fb = new DistributedFrameBuffer(ospray::mpi::async::CommLayer::WORLD,
                                                   size,
                                                   handle,
                                                   colorBufferFormat,
                                                   hasDepthBuffer,hasAccumBuffer);
      fb->refInc();
      ObjectHandle::assign(handle,fb);
      cmd.newCommand(CMD_FRAMEBUFFER_CREATE);
      cmd.send(handle);
      cmd.send(size);
      cmd.send((int32)mode);
      cmd.send((int32)channels);
      cmd.flush();
      return (OSPFrameBuffer)(int64)handle;
    }
    

    /*! map frame buffer */
    const void *MPIDevice::frameBufferMap(OSPFrameBuffer _fb, 
                                          OSPFrameBufferChannel channel)
    {
      int rc; 

      ObjectHandle handle = (const ObjectHandle &)_fb;
      FrameBuffer *fb = (FrameBuffer *)handle.lookup();

      LocalFrameBuffer *lfb = (LocalFrameBuffer*)fb;

      switch (channel) {
      case OSP_FB_COLOR: return fb->mapColorBuffer();
      case OSP_FB_DEPTH: return fb->mapDepthBuffer();
      default: return NULL;
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
      ObjectHandle handle = ObjectHandle::alloc();
      cmd.newCommand(CMD_NEW_MODEL);
      cmd.send(handle);
      cmd.flush();
      return (OSPModel)(int64)handle;
    }
    
    /*! finalize a newly specified model */
    void MPIDevice::commit(OSPObject _object)
    {
      Assert(_object);
      cmd.newCommand(CMD_COMMIT);
      const ObjectHandle handle = (const ObjectHandle&)_object;
      cmd.send(handle);
      cmd.flush();

      MPI_Barrier(MPI_COMM_WORLD);
    }
    
    /*! add a new geometry to a model */
    void MPIDevice::addGeometry(OSPModel _model, OSPGeometry _geometry)
    {
      Assert(_model);
      Assert(_geometry);
      cmd.newCommand(CMD_ADD_GEOMETRY);
      cmd.send((const ObjectHandle &)_model);
      cmd.send((const ObjectHandle &)_geometry);
      cmd.flush();
    }

    /*! add a new volume to a model */
    void MPIDevice::addVolume(OSPModel _model, OSPVolume _volume)
    {
      Assert(_model);
      Assert(_volume);
      cmd.newCommand(CMD_ADD_VOLUME);
      cmd.send((const ObjectHandle &) _model);
      cmd.send((const ObjectHandle &) _volume);
      cmd.flush();
    }

    /*! create a new data buffer */
    OSPData MPIDevice::newData(size_t nitems, OSPDataType format,
                               void *init, int flags)
    {
      ObjectHandle handle = ObjectHandle::alloc();
      cmd.newCommand(CMD_NEW_DATA);
      cmd.send(handle);
      cmd.send(nitems);
      cmd.send((int32)format);
      cmd.send((int32)flags);
      size_t size = init?ospray::sizeOf(format)*nitems:0;
      cmd.send(size);
      if (init) {
        cmd.send(init,size);
        if (format == OSP_OBJECT) {
          // no need to do anything special here: while we have to
          // encode objects as handles for network transfer, the host
          // _already_ has only handles, so whatever data was written
          // into the dta array are already handles.

          // note: we _might_, in fact, have to increaes the data
          // array entries' refcount here !?
        }
      }
      cmd.flush();
      return (OSPData)(int64)handle;
    }
        
    /*! assign (named) string parameter to an object */
    void MPIDevice::setVoidPtr(OSPObject _object, const char *bufName, void *v)
    {
      throw std::runtime_error("setting a void pointer as parameter to an "
                               "object is not allowed in MPI mode");
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

      Assert(type != OSP_UNKNOWN && "unknown volume voxel type");
      int typeSize = sizeOf(type);

      size_t size = typeSize * size_t(count.x) * size_t(count.y) * size_t(count.z);
      // This size restriction is imposed by MPI_Bcast which indexes into the
      // buffer with an int
      // limiting us to a max size of 2^31 bytes, a bit more than 2GB
      if (size > 2000000000LL) {
        throw std::runtime_error("setregion does not currently work for "
                                 "region sizes > 2GB");
      }

      cmd.newCommand(CMD_SET_REGION);
      cmd.send((const ObjectHandle &)_volume);
      cmd.send(index);
      cmd.send(count);
      cmd.send(size);
      cmd.send(source,size);
      cmd.flush();

      int numFails = 0;
      MPI_Status status;
      int rc = MPI_Recv(&numFails,1,MPI_INT,
                        0,MPI_ANY_TAG,mpi::worker.comm,&status);

      Assert(rc == MPI_SUCCESS);

      return (numFails == 0);
    }

    /*! assign (named) string parameter to an object */
    void MPIDevice::setString(OSPObject _object, const char *bufName, const char *s)
    {
      Assert(_object);
      Assert(bufName);

      cmd.newCommand(CMD_SET_STRING);
      cmd.send((const ObjectHandle &)_object);
      cmd.send(bufName);
      cmd.send(s);
    }

    /*! load module */
    int MPIDevice::loadModule(const char *name)
    {

#if THIS_IS_MIC
      // embree automatically puts this into "lib<name>.so" format
      std::string libName = "ospray_module_"+std::string(name)+"_mic";
#else
      std::string libName = "ospray_module_"+std::string(name)+"";
#endif
      loadLibrary(libName);

      std::string initSymName = "ospray_init_module_"+std::string(name);
      void*initSym = getSymbol(initSymName);
      if (!initSym)
        throw std::runtime_error("#osp:mpi:mpidevice: could not find module initializer "+initSymName);
      void (*initMethod)() = (void(*)())initSym;
      initMethod();

      cmd.newCommand(CMD_LOAD_MODULE);
      cmd.send(name);
      
      // FIXME: actually we should return an error code here...
      return 0;
    }

    /*! assign (named) float parameter to an object */
    void MPIDevice::setFloat(OSPObject _object, const char *bufName, const float f)
    {
      Assert(_object);
      Assert(bufName);

      cmd.newCommand(CMD_SET_FLOAT);
      cmd.send((const ObjectHandle &)_object);
      cmd.send(bufName);
      cmd.send(f);
    }

    /*! assign (named) int parameter to an object */
    void MPIDevice::setInt(OSPObject _object, const char *bufName, const int i)
    {
      Assert(_object);
      Assert(bufName);

      cmd.newCommand(CMD_SET_INT);
      cmd.send((const ObjectHandle &)_object);
      cmd.send(bufName);
      cmd.send(i);
    }

    /*! assign (named) vec2f parameter to an object */
    void MPIDevice::setVec2f(OSPObject _object, const char *bufName, const vec2f &v)
    {
      Assert(_object);
      Assert(bufName);

      cmd.newCommand(CMD_SET_VEC2F);
      cmd.send((const ObjectHandle &) _object);
      cmd.send(bufName);
      cmd.send(v);
    }

    /*! assign (named) vec3f parameter to an object */
    void MPIDevice::setVec3f(OSPObject _object, const char *bufName, const vec3f &v)
    {
      Assert(_object);
      Assert(bufName);

      cmd.newCommand(CMD_SET_VEC3F);
      cmd.send((const ObjectHandle &)_object);
      cmd.send(bufName);
      cmd.send(v);
    }

    /*! assign (named) vec4f parameter to an object */
    void MPIDevice::setVec4f(OSPObject _object, const char *bufName, const vec4f &v)
    {
      Assert(_object);
      Assert(bufName);

      cmd.newCommand(CMD_SET_VEC4F);
      cmd.send((const ObjectHandle &)_object);
      cmd.send(bufName);
      cmd.send(v);
    }

    /*! assign (named) vec2i parameter to an object */
    void MPIDevice::setVec2i(OSPObject _object, const char *bufName, const vec2i &v)
    {
      Assert(_object);
      Assert(bufName);

      cmd.newCommand(CMD_SET_VEC2I);
      cmd.send((const ObjectHandle &) _object);
      cmd.send(bufName);
      cmd.send(v);
    }

    /*! assign (named) vec3i parameter to an object */
    void MPIDevice::setVec3i(OSPObject _object, const char *bufName, const vec3i &v)
    {
      Assert(_object);
      Assert(bufName);

      cmd.newCommand(CMD_SET_VEC3I);
      cmd.send((const ObjectHandle &)_object);
      cmd.send(bufName);
      cmd.send(v);
    }

    /*! assign (named) data item as a parameter to an object */
    void MPIDevice::setObject(OSPObject _target, const char *bufName, OSPObject _value)
    {
      Assert(_target != NULL);
      Assert(bufName != NULL);
      const ObjectHandle tgtObjectHandle = (const ObjectHandle&)_target;
      const ObjectHandle valObjectHandle = (const ObjectHandle&)_value;

      cmd.newCommand(CMD_SET_OBJECT);
      cmd.send(tgtObjectHandle);
      cmd.send(bufName);
      cmd.send(valObjectHandle);
      cmd.flush();
    }

    /*! create a new pixelOp object (out of list of registered pixelOps) */
    OSPPixelOp MPIDevice::newPixelOp(const char *type)
    {
      Assert(type != NULL);

      ObjectHandle handle = ObjectHandle::alloc();

      cmd.newCommand(CMD_NEW_PIXELOP);
      cmd.send(handle);
      cmd.send(type);
      cmd.flush();
      return (OSPPixelOp)(int64)handle;
    }

    /*! set a frame buffer's pixel op object */
    void MPIDevice::setPixelOp(OSPFrameBuffer _fb, OSPPixelOp _op)
    {
      Assert(_fb != NULL);
      Assert(_op != NULL);

      cmd.newCommand(CMD_SET_PIXELOP);
      cmd.send((const ObjectHandle&)_fb);
      cmd.send((const ObjectHandle&)_op);
      cmd.flush();
    }
      
    /*! create a new renderer object (out of list of registered renderers) */
    OSPRenderer MPIDevice::newRenderer(const char *type)
    {
      Assert(type != NULL);

      ObjectHandle handle = ObjectHandle::alloc();

      cmd.newCommand(CMD_NEW_RENDERER);
      cmd.send(handle);
      cmd.send(type);
      cmd.flush();
      return (OSPRenderer)(int64)handle;
    }

    /*! create a new camera object (out of list of registered cameras) */
    OSPCamera MPIDevice::newCamera(const char *type)
    {
      Assert(type != NULL);

      ObjectHandle handle = ObjectHandle::alloc();

      cmd.newCommand(CMD_NEW_CAMERA);
      cmd.send(handle);
      cmd.send(type);
      cmd.flush();
      return (OSPCamera)(int64)handle;
    }

    /*! create a new volume object (out of list of registered volumes) */
    OSPVolume MPIDevice::newVolume(const char *type)
    {
      Assert(type != NULL);

      ObjectHandle handle = ObjectHandle::alloc();

      cmd.newCommand(CMD_NEW_VOLUME);
      cmd.send(handle);
      cmd.send(type);
      cmd.flush();
      return (OSPVolume)(int64)handle;
    }

    /*! create a new geometry object (out of list of registered geometries) */
    OSPGeometry MPIDevice::newGeometry(const char *type)
    {
      Assert(type != NULL);

      ObjectHandle handle = ObjectHandle::alloc();
      
      cmd.newCommand(CMD_NEW_GEOMETRY);
      cmd.send((const ObjectHandle&)handle);
      cmd.send(type);
      cmd.flush();
      return (OSPGeometry)(int64)handle;
    }
    
    /*! have given renderer create a new material */
    OSPMaterial MPIDevice::newMaterial(OSPRenderer _renderer, const char *type)
    {
      if (type == NULL)
        throw std::runtime_error("#osp:mpi:newMaterial: NULL material type");
      
      if (_renderer == NULL) 
        throw std::runtime_error("#osp:mpi:newMaterial: NULL renderer handle");

      ObjectHandle handle = ObjectHandle::alloc();
      
      cmd.newCommand(CMD_NEW_MATERIAL);
      cmd.send((const ObjectHandle&)_renderer);
      cmd.send((const ObjectHandle&)handle);

      cmd.send(type);
      cmd.flush();

      int numFails = 0;
      MPI_Status status;
      int rc = MPI_Recv(&numFails,1,MPI_INT,
                        0,MPI_ANY_TAG,mpi::worker.comm,&status);
      if (numFails == 0)
        return (OSPMaterial)(int64)handle;
      else {
        handle.free();
        return (OSPMaterial)NULL;
      }
    }

      /*! create a new transfer function object (out of list of 
        registered transfer function types) */
    OSPTransferFunction MPIDevice::newTransferFunction(const char *type)
    {
      Assert(type != NULL);

      ObjectHandle handle = ObjectHandle::alloc();

      cmd.newCommand(CMD_NEW_TRANSFERFUNCTION);
      cmd.send(handle);
      cmd.send(type);
      cmd.flush();
      return (OSPTransferFunction)(int64)handle;
    }


    /*! have given renderer create a new Light */
    OSPLight MPIDevice::newLight(OSPRenderer _renderer, const char *type)
    {
      if (type == NULL)
        throw std::runtime_error("#osp:mpi:newLight: NULL light type");

      ObjectHandle handle = ObjectHandle::alloc();
      
      cmd.newCommand(CMD_NEW_LIGHT);
      cmd.send((const ObjectHandle&)_renderer);
      cmd.send((const ObjectHandle&)handle);

      cmd.send(type);
      cmd.flush();

      // int MPI_Allreduce(void* , void*, int, MPI_Datatype, MPI_Op, MPI_Comm);
      int numFails = 0;
      MPI_Status status;
      int rc = MPI_Recv(&numFails,1,MPI_INT,
                        0,MPI_ANY_TAG,mpi::worker.comm,&status);
      if (numFails==0)
        return (OSPLight)(int64)handle;
      else {
        handle.free();
        return (OSPLight)NULL;
      }
      return NULL;
    }

    /*! clear the specified channel(s) of the frame buffer specified in 'whichChannels'
        
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
      cmd.newCommand(CMD_FRAMEBUFFER_CLEAR);
      cmd.send((const ObjectHandle&)_fb);
      cmd.send((int32)fbChannelFlags);
      cmd.flush();
    }

    /*! remove an existing geometry from a model */
    void MPIDevice::removeGeometry(OSPModel _model, OSPGeometry _geometry)
    {
      cmd.newCommand(CMD_REMOVE_GEOMETRY);
      cmd.send((const ObjectHandle&)_model);
      cmd.send((const ObjectHandle&)_geometry);
      cmd.flush();
    }

    /*! remove an existing volume from a model */
    void MPIDevice::removeVolume(OSPModel _model, OSPVolume _volume)
    {
      cmd.newCommand(CMD_REMOVE_VOLUME);
      cmd.send((const ObjectHandle&)_model);
      cmd.send((const ObjectHandle&)_volume);
      cmd.flush();
    }


    /*! call a renderer to render a frame buffer */
    float MPIDevice::renderFrame(OSPFrameBuffer _fb, 
                                OSPRenderer _renderer, 
                                const uint32 fbChannelFlags)
    {
      double T0 = getSysTime();
      const ObjectHandle handle = (const ObjectHandle&)_fb;
      // const ObjectHandle handle = (const ObjectHandle&)_sc;
      // SwapChain *sc = (SwapChain *)handle.lookup();
      FrameBuffer *fb = (FrameBuffer *)handle.lookup();
      // Assert(sc);

      //FrameBuffer *fb = sc->getFrontBuffer();
      // FrameBuffer *fb = sc->getBackBuffer();
      cmd.newCommand(CMD_RENDER_FRAME);
      cmd.send((const ObjectHandle&)_fb);
      cmd.send((const ObjectHandle&)_renderer);
      cmd.send((int32)fbChannelFlags);
      cmd.flush();

      return TiledLoadBalancer::instance->renderFrame(NULL,fb,fbChannelFlags);
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
      cmd.newCommand(CMD_RELEASE);
      cmd.send((const ObjectHandle&)_obj);
      cmd.flush();
    }

    //! assign given material to given geometry
    void MPIDevice::setMaterial(OSPGeometry _geometry, OSPMaterial _material)
    {
      cmd.newCommand(CMD_SET_MATERIAL);
      cmd.send((const ObjectHandle&)_geometry);
      cmd.send((const ObjectHandle&)_material);
      cmd.flush();
    }

    /*! create a new Texture2D object */
    OSPTexture2D MPIDevice::newTexture2D(const vec2i &sz,
        const OSPTextureFormat type, void *data, const uint32 flags)
    {
      ObjectHandle handle = ObjectHandle::alloc();
      cmd.newCommand(CMD_NEW_TEXTURE2D);
      cmd.send(handle);
      cmd.send(sz);
      cmd.send((int32)type);
      cmd.send((int32)flags);
      assert(data);
      size_t size = ospray::sizeOf(type) * sz.x * sz.y;
      cmd.send(size);

      cmd.send(data,size);
      cmd.flush();
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
        NOTIMPLEMENTED;
      } break;
        // ==================================================================
        // currently in default (mastered) mode where master tells workers what to do
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
          printf("rank %i telling clients to switch to %s mode.\n",mpi::world.rank,apiModeName(newMode));
          cmd.newCommand(CMD_API_MODE);
          cmd.send((int32)newMode);
          cmd.flush();
          currentApiMode = newMode;
          // and just to be sure, do a barrier here -- not acutally needed AFAICT.
          MPI_Barrier(MPI_COMM_WORLD);
        } break;
        default:
          NOTIMPLEMENTED;
        };
      } break;
        // ==================================================================
        // ==================================================================
      case OSPD_MODE_COLLABORATIVE: {
        NOTIMPLEMENTED;
      } break;
        
        // ==================================================================
        // this mode should not exit - implementation error
        // ==================================================================
      default:
        NOTIMPLEMENTED;
      };
      throw std::runtime_error("Distributed API not available on this device "
                               "(when calling ospApiMode())");
    }

    void MPIDevice::sampleVolume(float **results, OSPVolume volume, 
                                 const vec3f *worldCoordinates, const size_t &count)
    {
      Assert2(volume, "invalid volume handle");
      Assert2(worldCoordinates, "invalid worldCoordinates");

      cmd.newCommand(CMD_SAMPLE_VOLUME);
      cmd.send((const ObjectHandle &) volume);
      cmd.send(count);
      cmd.send(worldCoordinates, count * sizeof(osp::vec3f));
      cmd.flush();

      *results = (float *)malloc(count * sizeof(float));
      Assert(*results);

      // for data-distributed volumes this will need to be updated...
      cmd.get_data(count * sizeof(float), *results, 0, mpi::worker.comm);
    }

    int MPIDevice::getString(OSPObject object, const char *name, char **value)
    {
      Assert(object);
      Assert(name);

      cmd.newCommand(CMD_GET_VALUE);
      cmd.send((const ObjectHandle &) object);
      cmd.send(name);
      cmd.send(OSP_STRING);
      cmd.flush();

      struct ReturnValue { int success; char value[2048]; } result;
      cmd.get_data(sizeof(ReturnValue), &result, 0, mpi::worker.comm);

      return result.success ? *value = strdup(result.value), true : false;
    }

  } // ::ospray::mpi
} // ::ospray

