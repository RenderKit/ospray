#include "localdevice.h"
#include "../fb/swapchain.h"
#include "../common/model.h"
#include "../common/data.h"
#include "../geometry/trianglemesh.h"
#include "../render/renderer.h"
#include "../camera/camera.h"
#include "../volume/volume.h"
#include "../render/loadbalancer.h"

namespace ospray {
  namespace api {

    LocalDevice::LocalDevice(int *_ac, const char **_av)
    {
      char *logLevelFromEnv = getenv("OSPRAY_LOG_LEVEL");
      if (logLevelFromEnv) 
        logLevel = atoi(logLevelFromEnv);
      else
        logLevel = 0;

      ospray::init(_ac,&_av);

      // initialize embree. (we need to do this here rather than in
      // ospray::init() because in mpi-mode the latter is also called
      // in the host-stubs, where it shouldn't.
      std::stringstream embreeConfig;
      if (debugMode)
        embreeConfig << " threads=1";
      rtcInit(embreeConfig.str().c_str());
      assert(rtcGetError() == RTC_NO_ERROR);
      TiledLoadBalancer::instance = new LocalTiledLoadBalancer;
    }


    OSPFrameBuffer 
    LocalDevice::frameBufferCreate(const vec2i &size, 
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
    const void *LocalDevice::frameBufferMap(OSPFrameBuffer fb)
    {
      Assert(fb != NULL);
      SwapChain *sc = (SwapChain *)fb;
      return sc->map();
    }

    /*! unmap previously mapped frame buffer */
    void LocalDevice::frameBufferUnmap(const void *mapped,
                                       OSPFrameBuffer fb)
    {
      Assert2(fb != NULL, "invalid framebuffer");
      SwapChain *sc = (SwapChain *)fb;
      sc->unmap(mapped);
    }

    /*! create a new model */
    OSPModel LocalDevice::newModel()
    {
      Model *model = new Model;
      model->refInc();
      return (OSPModel)model;
    }
    
    /*! finalize a newly specified model */
    void LocalDevice::commit(OSPObject _object)
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
    void LocalDevice::addGeometry(OSPModel _model, OSPGeometry _geometry)
    {
      Model *model = (Model *)_model;
      Assert2(model,"null model in LocalDevice::finalizeModel()");

      Geometry *geometry = (Geometry *)_geometry;
      Assert2(geometry,"null geometry in LocalDevice::finalizeGeometry()");

      model->geometry.push_back(geometry);
    }

    /*! create a new data buffer */
    OSPTriangleMesh LocalDevice::newTriangleMesh()
    {
      TriangleMesh *triangleMesh = new TriangleMesh;
      triangleMesh->refInc();
      return (OSPTriangleMesh)triangleMesh;
    }

    /*! create a new data buffer */
    OSPData LocalDevice::newData(size_t nitems, OSPDataType format, void *init, int flags)
    {
      Assert2(flags == 0,"unsupported combination of flags");
      Data *data = new Data(nitems,format,init,flags);
      data->refInc();
      return (OSPData)data;
    }
    
    /*! assign (named) string parameter to an object */
    void LocalDevice::setString(OSPObject _object, const char *bufName, const char *s)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != NULL  && "invalid object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");
      object->findParam(bufName,1)->set(s);
    }

    /*! assign (named) int parameter to an object */
    void LocalDevice::setInt(OSPObject _object, const char *bufName, const int f)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != NULL  && "invalid object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");

      object->findParam(bufName,1)->set(f);
    }
    /*! assign (named) float parameter to an object */
    void LocalDevice::setFloat(OSPObject _object, const char *bufName, const float f)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != NULL  && "invalid object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");

      object->findParam(bufName,1)->set(f);
    }

    /*! assign (named) vec3f parameter to an object */
    void LocalDevice::setVec3f(OSPObject _object, const char *bufName, const vec3f &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != NULL  && "invalid object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");

      object->findParam(bufName,1)->set(v);
    }
    /*! assign (named) vec3i parameter to an object */
    void LocalDevice::setVec3i(OSPObject _object, const char *bufName, const vec3i &v)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Assert(object != NULL  && "invalid object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");

      object->findParam(bufName,1)->set(v);
    }

    /*! assign (named) data item as a parameter to an object */
    void LocalDevice::setObject(OSPObject _target, const char *bufName, OSPObject _value)
    {
      ManagedObject *target = (ManagedObject *)_target;
      ManagedObject *value  = (ManagedObject *)_value;

      Assert(target != NULL  && "invalid target object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");

      target->setParam(bufName,value);
    }

    /*! create a new renderer object (out of list of registered renderers) */
    OSPRenderer LocalDevice::newRenderer(const char *type)
    {
      Assert(type != NULL && "invalid render type identifier");
      Renderer *renderer = Renderer::createRenderer(type);
      renderer->refInc();
      return (OSPRenderer)renderer;
    }

    /*! create a new geometry object (out of list of registered geometrys) */
    OSPGeometry LocalDevice::newGeometry(const char *type)
    {
      Assert(type != NULL && "invalid render type identifier");
      Geometry *geometry = Geometry::createGeometry(type);
      geometry->refInc();
      return (OSPGeometry)geometry;
    }

    /*! create a new camera object (out of list of registered cameras) */
    OSPCamera LocalDevice::newCamera(const char *type)
    {
      Assert(type != NULL && "invalid camera type identifier");
      Camera *camera = Camera::createCamera(type);
      camera->refInc();
      return (OSPCamera)camera;
    }

    /*! create a new volume object (out of list of registered volumes) */
    OSPVolume LocalDevice::newVolume(const char *type)
    {
      Assert(type != NULL && "invalid volume type identifier");
      /*! we don't have the volume interface fleshed out, yet, and
        currently create the proper volume type on-the-fly during
        commit, so use this wrpper thingy...  \see
        volview_notes_on_volume_interface */
      WrapperVolume *volume = new WrapperVolume; 
      volume->refInc();
      return (OSPVolume)volume;
    }

    /*! call a renderer to render a frame buffer */
    void LocalDevice::renderFrame(OSPFrameBuffer _sc, 
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

