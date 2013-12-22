#include "localdevice.h"
#include "../fb/swapchain.h"
#include "../common/model.h"
#include "../common/data.h"
#include "../geometry/trianglemesh.h"
#include "../render/renderer.h"

namespace ospray {
  namespace api {

    LocalDevice::LocalDevice()
    {
      char *logLevelFromEnv = getenv("OSPRAY_LOG_LEVEL");
      if (logLevelFromEnv) 
        logLevel = atoi(logLevelFromEnv);
      else
        logLevel = 0;
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
      PING;
      PRINT(sc);
      PRINT(sc->toString());
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
      return sc->unmap(mapped);
    }

    /*! create a new model */
    OSPModel LocalDevice::newModel()
    {
      Model *model = new Model;
      model->refInc();
      return (OSPModel)model;
    }
    
    /*! finalize a newly specified model */
    void LocalDevice::finalizeModel(OSPModel _model)
    {
      Model *model = (Model *)_model;
      Assert2(model,"null model in LocalDevice::finalizeModel()");
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
    
    /*! assign (named) data item as a parameter to an object */
    void LocalDevice::setData(OSPObject _object, const char *bufName, OSPData _data)
    {
      ManagedObject *object = (ManagedObject *)_object;
      Data          *data   = (Data *)_data;

      Assert(data != NULL && "invalid data array handle");
      Assert(object != NULL && "invalid object handle");
      Assert(bufName != NULL && "invalid identifier for object parameter");

      object->setParam(bufName,data);
    }

      /*! create a new renderer object (out of list of registered renderers) */
    OSPRenderer LocalDevice::newRenderer(const char *type)
    {
      PING;
      Assert(type != NULL && "invalid render type identifier");
      Renderer *renderer = Renderer::createRenderer(type);
      return (OSPRenderer)renderer;
    }

      /*! call a renderer to render a frame buffer */
    void LocalDevice::renderFrame(OSPFrameBuffer _sc, 
                                  OSPRenderer    _renderer, 
                                  OSPModel       _model)
    {
      SwapChain *sc = (SwapChain *)_sc;
      Renderer *renderer = (Renderer *)_renderer;
      Model *model = (Model *)_model;

      Assert(sc != NULL && "invalid frame buffer handle");
      Assert(renderer != NULL && "invalid renderer handle");
      
      FrameBuffer *fb = sc->getBackBuffer();
      renderer->renderFrame(fb,model);
    }

  }
}

