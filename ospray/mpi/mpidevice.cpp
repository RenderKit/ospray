#include "mpicommon.h"
#include "mpidevice.h"
#include "../fb/swapchain.h"
#include "../common/model.h"
#include "../common/data.h"
#include "../geometry/trianglemesh.h"
#include "../render/renderer.h"
#include "../camera/camera.h"
#include "../volume/volume.h"
#include "mpiloadbalancer.h"

namespace ospray {
  using std::cout;
  using std::endl;

  namespace mpi {
    Group world;
    Group app;
    Group worker;

    //! this runs an ospray worker process. 
    /*! it's up to the proper init
      routine to decide which processes call this function and which
      ones don't. This function will not return. */
    void runWorker(int *_ac, const char **_av);
    
    /*! in this mode ("ospray on ranks" mode, or "ranks" mode)
      - the user has launched mpirun explicitly, and all processes are *already* running
      - the app is supposed to be running *only* on the root process
      - ospray is supposed to be running on all ranks *other than* the root
      - "ranks" means all processes with world.rank >= 1
      
      For this function, we assume: 
      - all *all* MPI_COMM_WORLD processes are going into this function
    */
    void initMPI_OSPonRanks(int *ac, const char **av)
    {
      MPI_Status status;
      mpi::init(ac,av);
      printf("#o: initMPI::OSPonRanks: %i/%i\n",world.rank,world.size);
      MPI_Barrier(MPI_COMM_WORLD);

      Assert(world.size > 1);
      if (world.rank == 0) {
        // we're the root
        MPI_Comm_split(mpi::world.comm,1,mpi::world.rank,&app.comm);
        app.makeIntercomm();
        printf("#w: app process %i/%i (global %i/%i)\n",app.rank,app.size,world.rank,world.size);

        MPI_Intercomm_create(app.comm, 0, world.comm, 1, 1, &worker.comm); 
        worker.makeIntracomm();

        printf("#m: ping-ponging a test message to every worker...\n");
        for (int i=0;i<worker.size;i++) {
          printf("#m: sending tag %i to worker %i\n",i,i);
          MPI_Send(&i,1,MPI_INT,i,i,worker.comm);
          int reply;
          MPI_Recv(&reply,1,MPI_INT,i,i,worker.comm,&status);
          Assert(reply == i);
        }
      } else {
        // we're the workers
        MPI_Comm_split(mpi::world.comm,0,mpi::world.rank,&worker.comm);
        worker.makeIntercomm();
        printf("#w: worker process %i/%i (global %i/%i)\n",worker.rank,worker.size,world.rank,world.size);

        MPI_Intercomm_create(worker.comm, 0, world.comm, 0, 1, &app.comm); 
        app.makeIntracomm();
        // worker.containsMe = true;
        // app.containsMe = false;

        // replying to test-message
        printf("#w: worker %i trying to receive tag %i...\n",worker.rank,worker.rank);
        int reply;
        MPI_Recv(&reply,1,MPI_INT,0,worker.rank,app.comm,&status);
        MPI_Send(&reply,1,MPI_INT,0,worker.rank,app.comm);
        MPI_Barrier(worker.comm);
      }

      MPI_Barrier(MPI_COMM_WORLD);
      // -------------------------------------------------------
      // at this point, all processes should be set up and synced. in particular
      // - app has intracommunicator to all workers (and vica versa)
      // - app process(es) are in one intercomm ("app"); workers all in another ("worker")
      // - all processes (incl app) have barrier'ed, and thus now in sync.

      // -------------------------------------------------------
      // now, 
      // - all workers will enter their worker loop (ie, they will *not* return
      // - root proc(s) will return, initialize the MPI device, then return to the app
      if (worker.containsMe)  
        mpi::runWorker(ac,av);
    }

    /*! in this mode ("local cluster" mode)
      - the user did _not_ launch any MPI explicitly
      - the app is running on a standalone process
      - osp will launch multiple worker processes that communicate w/ app throu intercomm
     */
    void initMPI_LaunchLocalCluster()
    {
      int rc;
      MPI_Status status;

      Assert(world.size == 1);
      app.comm = world.comm; 
      app.makeIntracomm();
      
      std::string launchCmd = "/tmp/launchit.sh";
      
      std::stringstream systemCmd;
      systemCmd
        << launchCmd << " "
        << endl;
      FILE *launchOutput = fopen(systemCmd.str().c_str(),"r");
      Assert(launchOutput);
      char line[10000];
      char *listenPort = NULL;
      const char *LAUNCH_HEADER = "OSPRAY_SERVICE_PORT:";
      while (fgets(line,10000,launchOutput) && !feof(launchOutput)) {
        if (!strncmp(line,LAUNCH_HEADER,strlen(LAUNCH_HEADER))) {
          listenPort = line + strlen(LAUNCH_HEADER);
          char *eol = strstr(listenPort,"\n");
          if (eol) *eol = 0;
          break;
        }
      }
      fclose(launchOutput);
      if (!listenPort)
        throw std::runtime_error("failed to find service port in launch script output");

      rc = MPI_Comm_connect(listenPort,MPI_INFO_NULL,0,app.comm,&worker.comm);
      worker.makeIntercomm();
    }
    
    /*! in this mode
      - ther user runs a distributed app (a la paraview)
      - MPI processes have already been launched by user
      - each process with run its own local instance of the app process
      - each app will use ospray *locally* (ie, osp will *not* start anything new other 
        than threads 
    */
    void initMPI_DistributedAppWithLocalOSP();

  }

  namespace api {
    Device *createDevice_MPI_OSPonRanks(int *ac, const char **av)
    {
      mpi::initMPI_OSPonRanks(ac,av);
      // only the app process(es) will still reach this point.
      return new MPIDevice(ac,av);
    }

    MPIDevice::MPIDevice(// AppMode appMode, OSPMode ospMode,
                         int *_ac, const char **_av)
    {
      char *logLevelFromEnv = getenv("OSPRAY_LOG_LEVEL");
      if (logLevelFromEnv) 
        logLevel = atoi(logLevelFromEnv);
      else
        logLevel = 0;

      // Assert2(_ac,"no params");
      // Assert2(*_ac > 1, "no service handle specified after '--mpi'");
      // const char *serviceSocket = strdup(_av[1]);
      // removeArgs(*_ac,(char**&)_av,1,1);

      ospray::init(_ac,&_av);
      // mpi::init(_ac,_av);

      if (mpi::world.size !=1) {
        if (mpi::world.rank != 0) {
          PRINT(mpi::world.rank);
          PRINT(mpi::world.size);
          throw std::runtime_error("OSPRay MPI startup error. Use \"mpirun -n 1 <command>\" when calling an application that tries to spawnto start the application you were trying to start.");
        }
      }

      TiledLoadBalancer::instance = new mpi::DynamicLoadBalancer_Master;
      PING;
    }


    OSPFrameBuffer 
    MPIDevice::frameBufferCreate(const vec2i &size, 
                                 const OSPFrameBufferMode mode,
                                 const size_t swapChainDepth)
    {
      FrameBufferFactory fbFactory = NULL;
      switch(mode) {
      case OSP_RGBA_I8:
        fbFactory = createLocalFB_RGBA_I8;
        break;
      default:
        AssertError("frame buffer mode not yet supported");
      }
      SwapChain *sc = new SwapChain(swapChainDepth,size,fbFactory);
      Assert(sc != NULL);
      
      mpi::Handle handle = mpi::Handle::alloc();
      mpi::Handle::assign(handle,sc);
      // mpi::objectByID[handle] = sc;
      
      cmd.newCommand(CMD_FRAMEBUFFER_CREATE);
      cmd.send(handle);
      cmd.send(size);
      cmd.send((int32)mode);
      cmd.send((int32)swapChainDepth);
      cmd.flush();
      return (OSPFrameBuffer)(int64)handle;
    }
    

      /*! map frame buffer */
    const void *MPIDevice::frameBufferMap(OSPFrameBuffer fb)
    {
      int rc; 
      MPI_Status status;

      mpi::Handle handle = (const mpi::Handle &)fb;
      SwapChain *sc = (SwapChain *)handle.lookup();
      Assert(sc);

      cmd.newCommand(CMD_FRAMEBUFFER_MAP);
      cmd.send(handle);
      cmd.flush();
      void *ptr = (void*)sc->map();
      rc = MPI_Recv(ptr,sc->fbSize.x*sc->fbSize.y,MPI_INT,0,0,mpi::worker.comm,&status);
      Assert(rc == MPI_SUCCESS);
      return ptr;
    }

    /*! unmap previously mapped frame buffer */
    void MPIDevice::frameBufferUnmap(const void *mapped,
                                       OSPFrameBuffer fb)
    {
      mpi::Handle handle = (const mpi::Handle &)fb;
      SwapChain *sc = (SwapChain *)handle.lookup();
      Assert(sc);
      sc->unmap(mapped);
    }

    /*! create a new model */
    OSPModel MPIDevice::newModel()
    {
      mpi::Handle handle = mpi::Handle::alloc();
      cmd.newCommand(CMD_NEW_MODEL);
      cmd.send(handle);
      cmd.flush();
      return (OSPModel)(int64)handle;
    }
    
    // /*! finalize a newly specified model */
    // void MPIDevice::finalizeModel(OSPModel _model)
    // {
    //   Model *model = (Model *)_model;
    //   Assert2(model,"null model in MPIDevice::finalizeModel()");
    //   model->finalize();
    // }

    /*! finalize a newly specified model */
    void MPIDevice::commit(OSPObject _object)
    {
      Assert(_object);
      cmd.newCommand(CMD_COMMIT);
      const mpi::Handle handle = (const mpi::Handle&)_object;
      cmd.send((const mpi::Handle&)_object);
      cmd.flush();
    }
    
    /*! add a new geometry to a model */
    void MPIDevice::addGeometry(OSPModel _model, OSPGeometry _geometry)
    {
      Assert(_model);
      Assert(_geometry);
      cmd.newCommand(CMD_ADD_GEOMETRY);
      cmd.send((const mpi::Handle &)_model);
      cmd.send((const mpi::Handle &)_geometry);
      cmd.flush();
    }

    /*! create a new data buffer */
    OSPTriangleMesh MPIDevice::newTriangleMesh()
    {
      mpi::Handle handle = mpi::Handle::alloc();
      cmd.newCommand(CMD_NEW_TRIANGLEMESH);
      cmd.send(handle);
      cmd.flush();
      return (OSPTriangleMesh)(int64)handle;
      // NOTIMPLEMENTED;
      // TriangleMesh *triangleMesh = new TriangleMesh;
      // triangleMesh->refInc();
      // return (OSPTriangleMesh)triangleMesh;
    }

    /*! create a new data buffer */
    OSPData MPIDevice::newData(size_t nitems, OSPDataType format, void *init, int flags)
    {
      mpi::Handle handle = mpi::Handle::alloc();
      cmd.newCommand(CMD_NEW_DATA);
      cmd.send(handle);
      cmd.send(nitems);
      cmd.send((int32)format);
      cmd.send(flags);
      size_t size = init?ospray::sizeOf(format)*nitems:0;
      cmd.send(size);
      if (init) {
        cmd.send(init,size);
      }
      cmd.flush();
      return (OSPData)(int64)handle;
    }
    
    
    /*! assign (named) string parameter to an object */
    void MPIDevice::setString(OSPObject _object, const char *bufName, const char *s)
    {
      NOTIMPLEMENTED;
      // ManagedObject *object = (ManagedObject *)_object;
      // Assert(object != NULL  && "invalid object handle");
      // Assert(bufName != NULL && "invalid identifier for object parameter");
      
      // PING;
      // PRINT(bufName);
      // PRINT(s);

      // object->findParam(bufName,1)->set(s);
    }

    /*! assign (named) float parameter to an object */
    void MPIDevice::setFloat(OSPObject _object, const char *bufName, const float f)
    {
      Assert(_object);
      Assert(bufName);

      cmd.newCommand(CMD_SET_FLOAT);
      cmd.send((const mpi::Handle &)_object);
      cmd.send(bufName);
      cmd.send(f);
    }

    /*! assign (named) int parameter to an object */
    void MPIDevice::setInt(OSPObject _object, const char *bufName, const int f)
    {
      NOTIMPLEMENTED;
      // ManagedObject *object = (ManagedObject *)_object;
      // Assert(object != NULL  && "invalid object handle");
      // Assert(bufName != NULL && "invalid identifier for object parameter");

      // object->findParam(bufName,1)->set(f);
    }

    /*! assign (named) vec3f parameter to an object */
    void MPIDevice::setVec3f(OSPObject _object, const char *bufName, const vec3f &v)
    {
      Assert(_object);
      Assert(bufName);

      cmd.newCommand(CMD_SET_VEC3F);
      cmd.send((const mpi::Handle &)_object);
      cmd.send(bufName);
      cmd.send(v);
    }

    /*! assign (named) vec3i parameter to an object */
    void MPIDevice::setVec3i(OSPObject _object, const char *bufName, const vec3i &v)
    {
      NOTIMPLEMENTED;
      // ManagedObject *object = (ManagedObject *)_object;
      // Assert(object != NULL  && "invalid object handle");
      // Assert(bufName != NULL && "invalid identifier for object parameter");

      // object->findParam(bufName,1)->set(v);
    }

    /*! assign (named) data item as a parameter to an object */
    void MPIDevice::setObject(OSPObject _target, const char *bufName, OSPObject _value)
    {
      Assert(_target != NULL);
      Assert(bufName != NULL);
      const mpi::Handle tgtHandle = (const mpi::Handle&)_target;
      const mpi::Handle valHandle = (const mpi::Handle&)_value;

      cmd.newCommand(CMD_SET_OBJECT);
      cmd.send(tgtHandle);
      cmd.send(bufName);
      cmd.send(valHandle);
      cmd.flush();
    }

      /*! create a new renderer object (out of list of registered renderers) */
    OSPRenderer MPIDevice::newRenderer(const char *type)
    {
      Assert(type != NULL);

      mpi::Handle handle = mpi::Handle::alloc();

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

      mpi::Handle handle = mpi::Handle::alloc();

      cmd.newCommand(CMD_NEW_CAMERA);
      cmd.send(handle);
      cmd.send(type);
      cmd.flush();
      return (OSPCamera)(int64)handle;
    }

    /*! create a new volume object (out of list of registered volumes) */
    OSPVolume MPIDevice::newVolume(const char *type)
    {
      NOTIMPLEMENTED;
      // Assert(type != NULL && "invalid render type identifier");
      // /*! we don't have the volume interface fleshed out, yet, and
      //   currently create the proper volume type on-the-fly during
      //   commit, so use this wrpper thingy...  \see
      //   volview_notes_on_volume_interface */
      // WrapperVolume *volume = new WrapperVolume; 
      // return (OSPVolume)(int64)volume;
    }

    /*! create a new geometry object (out of list of registered geometrys) */
    OSPGeometry MPIDevice::newGeometry(const char *type)
    {
      NOTIMPLEMENTED;
      // Assert(type != NULL && "invalid render type identifier");
      // Geometry *geometry = Geometry::createGeometry(type);
      // return (OSPGeometry)geometry;
    }


    /*! call a renderer to render a frame buffer */
    void MPIDevice::renderFrame(OSPFrameBuffer _sc, 
                                OSPRenderer    _renderer)
    {
      const mpi::Handle handle = (const mpi::Handle&)_sc;
      SwapChain *sc = (SwapChain *)handle.lookup();
      Assert(sc);

      FrameBuffer *fb = sc->getBackBuffer();
      cmd.newCommand(CMD_RENDER_FRAME);
      cmd.send((const mpi::Handle&)_sc);
      cmd.send((const mpi::Handle&)_renderer);
      cmd.flush();
      TiledLoadBalancer::instance->renderFrame(NULL,fb);

      // WARNING: I'm doing an *im*plicit swapbuffers here at the end
      // of renderframe, but to be more opengl-conform we should
      // actually have the user call an *ex*plicit ospSwapBuffers call...
      sc->advance();
    }
  }
}

