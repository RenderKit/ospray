#include "mpidevice.h"
#include "../fb/swapchain.h"
#include "../common/model.h"
#include "../common/data.h"
#include "../geometry/trianglemesh.h"
#include "../render/renderer.h"
#include "../camera/camera.h"

namespace ospray {
  namespace api {

    Device *createDevice_MPI(int *ac, const char **av)
    {
      return new MPIDevice(ac,av);
    }

    MPIDevice::MPIDevice(int *_ac, const char **_av)
    {
      char *logLevelFromEnv = getenv("OSPRAY_LOG_LEVEL");
      if (logLevelFromEnv) 
        logLevel = atoi(logLevelFromEnv);
      else
        logLevel = 0;

      ospray::init(_ac,&_av);


      MPI_Info info;
      printf("creating info\n");
      MPI_Info_create(&info);
      const char *hostlist = "localhost";
      MPI_Info_set(info,"host",(char*)hostlist); 
      printf("Spawning!\n");

      MPI_Comm_spawn("osp_mpi_worker", MPI_ARGV_NULL, 2,  
                     info, 0, MPI_COMM_SELF, &service,  
                     MPI_ERRCODES_IGNORE); 

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
      return (OSPFrameBuffer)sc;
    }
    

      /*! map frame buffer */
    const void *MPIDevice::frameBufferMap(OSPFrameBuffer fb)
    {
      Assert(fb != NULL);
      SwapChain *sc = (SwapChain *)fb;
      return sc->map();
    }

    /*! unmap previously mapped frame buffer */
    void MPIDevice::frameBufferUnmap(const void *mapped,
                                       OSPFrameBuffer fb)
    {
      Assert2(fb != NULL, "invalid framebuffer");
      SwapChain *sc = (SwapChain *)fb;
      return sc->unmap(mapped);
    }

    /*! create a new model */
    OSPModel MPIDevice::newModel()
    {
      Model *model = new Model;
      model->refInc();
      return (OSPModel)model;
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
      ManagedObject *object = (ManagedObject *)_object;
      Assert2(object,"null object in LocalDevice::commit()");
      object->commit();

      // hack, to stay compatible with earlier version
      Model *model = dynamic_cast<Model *>(object);
      if (model)
        model->finalize();
    }
    
    
    /*! add a new geometry to a model */
    void MPIDevice::addGeometry(OSPModel _model, OSPGeometry _geometry)
    {
      Model *model = (Model *)_model;
      Assert2(model,"null model in MPIDevice::finalizeModel()");

      Geometry *geometry = (Geometry *)_geometry;
      Assert2(geometry,"null geometry in MPIDevice::finalizeGeometry()");

      model->geometry.push_back(geometry);
    }

    /*! create a new data buffer */
    OSPTriangleMesh MPIDevice::newTriangleMesh()
    {
      TriangleMesh *triangleMesh = new TriangleMesh;
      triangleMesh->refInc();
      return (OSPTriangleMesh)triangleMesh;
    }

    /*! create a new data buffer */
    OSPData MPIDevice::newData(size_t nitems, OSPDataType format, void *init, int flags)
    {
      Assert2(flags == 0,"unsupported combination of flags");
      Data *data = new Data(nitems,format,init,flags);
      data->refInc();
      return (OSPData)data;
    }
    
    
    /*! assign (named) vec3f parameter to an object */
    void MPIDevice::setFloat(OSPObject _object, const char *bufName, const float f)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != NULL  && "invalid object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");

      object->findParam(bufName,1)->set(f);
    }

    /*! assign (named) vec3f parameter to an object */
    void MPIDevice::setVec3f(OSPObject _object, const char *bufName, const vec3f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != NULL  && "invalid object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");

      object->findParam(bufName,1)->set(v);
    }

    /*! assign (named) data item as a parameter to an object */
    void MPIDevice::setObject(OSPObject _target, const char *bufName, OSPObject _value)
    {
      ManagedObject *target = (ManagedObject *)_target;
      ManagedObject *value  = (ManagedObject *)_value;

      Assert(target != NULL  && "invalid target object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");

      target->setParam(bufName,value);
    }

      /*! create a new renderer object (out of list of registered renderers) */
    OSPRenderer MPIDevice::newRenderer(const char *type)
    {
      PING;
      Assert(type != NULL && "invalid render type identifier");
      Renderer *renderer = Renderer::createRenderer(type);
      return (OSPRenderer)renderer;
    }

    /*! create a new camera object (out of list of registered cameras) */
    OSPCamera MPIDevice::newCamera(const char *type)
    {
      Assert(type != NULL && "invalid render type identifier");
      Camera *camera = Camera::createCamera(type);
      return (OSPCamera)camera;
    }

    /*! call a renderer to render a frame buffer */
    void MPIDevice::renderFrame(OSPFrameBuffer _sc, 
                                  OSPRenderer    _renderer)
    {
      SwapChain *sc = (SwapChain *)_sc;
      Renderer *renderer = (Renderer *)_renderer;
      // Model *model = (Model *)_model;
      
      Assert(sc != NULL && "invalid frame buffer handle");
      Assert(renderer != NULL && "invalid renderer handle");
      
      FrameBuffer *fb = sc->getBackBuffer();
      renderer->renderFrame(fb);
      
      // WARNING: I'm doing an *im*plicit swapbuffers here at the end
      // of renderframe, but to be more opengl-conform we should
      // actually have the user call an *ex*plicit ospSwapBuffers call...
      sc->advance();
    }
  }
}

