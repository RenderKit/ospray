#undef NDEBUG

// OSPRay
#include "device.h"
#include "coidevice_common.h"
#include "common/data.h"
// COI
#include "common/COIResult_common.h"
#include "source/COIEngine_source.h"
#include "source/COIEvent_source.h"
#include "source/COIProcess_source.h"
#include "source/COIBuffer_source.h"
#include "source/COIPipeline_source.h"
//std
#include <map>


#include "../fb/tilesize.h"


//#define FORCE_SINGLE_DEVICE 1

namespace ospray {
  namespace coi {

#define SLEEP_DELAY
    //#define SLEEP_DELAY usleep(1000);

    const char *coiWorker = "./ospray_coi_worker.mic";

    void coiError(COIRESULT result, const std::string &err);

    struct COIDevice;

    typedef enum { 
      OSPCOI_NEW_MODEL=0,
      OSPCOI_NEW_DATA,
      OSPCOI_NEW_TRIANGLEMESH,
      OSPCOI_COMMIT,
      OSPCOI_SET_VALUE,
      OSPCOI_NEW_MATERIAL,
      OSPCOI_SET_MATERIAL,
      OSPCOI_NEW_CAMERA,
      OSPCOI_NEW_VOLUME,
      OSPCOI_NEW_RENDERER,
      OSPCOI_NEW_GEOMETRY,
      OSPCOI_ADD_GEOMETRY,
      OSPCOI_NEW_FRAMEBUFFER,
      OSPCOI_RENDER_FRAME,
      OSPCOI_RENDER_FRAME_SYNC,
      OSPCOI_NUM_FUNCTIONS
    } RemoteFctID;
    const char *coiFctName[] = {
      "ospray_coi_new_model",
      "ospray_coi_new_data",
      "ospray_coi_new_trianglemesh",
      "ospray_coi_commit",
      "ospray_coi_set_value",
      "ospray_coi_new_material",
      "ospray_coi_set_material",
      "ospray_coi_new_camera",
      "ospray_coi_new_volume",
      "ospray_coi_new_renderer",
      "ospray_coi_new_geometry",
      "ospray_coi_add_geometry",
      "ospray_coi_new_framebuffer",
      "ospray_coi_render_frame",
      "ospray_coi_render_frame_sync",
      NULL
    };
    
    struct COIEngine {
      COIENGINE       coiEngine;   // COI engine handle
      COIPIPELINE     coiPipe;     // COI pipeline handle
      COIPROCESS      coiProcess; 
      // COIDEVICE       coiDevice;   // COI device handle
      COI_ENGINE_INFO coiInfo;
      size_t          engineID;
      COIEVENT        event;
      
      COIDevice *osprayDevice;
      COIFUNCTION coiFctHandle[OSPCOI_NUM_FUNCTIONS];
      
      /*! create this engine, initialize coi with this engine ID, print
        basic device info */
      COIEngine(COIDevice *osprayDevice, size_t engineID);
      /*! load ospray worker onto device, and initialize basic ospray
        service */
      void loadOSPRay(); 
      void callFunction(RemoteFctID ID, const DataStream &data, bool sync);
    };

    struct COIDevice : public ospray::api::Device {
      std::vector<COIEngine *> engine;
      api::Handle handle;

      COIDevice();

      void callFunction(RemoteFctID ID, const DataStream &data, bool sync=true)
      { for (int i=0;i<engine.size();i++) engine[i]->callFunction(ID,data,sync); }

      /*! create a new frame buffer */
      virtual OSPFrameBuffer frameBufferCreate(const vec2i &size, 
                                               const OSPFrameBufferMode mode,
                                               const size_t swapChainDepth);

      /*! map frame buffer */
      virtual const void *frameBufferMap(OSPFrameBuffer fb);

      /*! unmap previously mapped frame buffer */
      virtual void frameBufferUnmap(const void *mapped,
                                    OSPFrameBuffer fb);

      /*! create a new model */
      virtual OSPModel newModel();

      // /*! finalize a newly specified model */
      // virtual void finalizeModel(OSPModel _model) { NOTIMPLEMENTED; }

      /*! commit the given object's outstanding changes */
      virtual void commit(OSPObject object);

      /*! add a new geometry to a model */
      virtual void addGeometry(OSPModel _model, OSPGeometry _geometry);

      /*! create a new data buffer */
      virtual OSPData newData(size_t nitems, OSPDataType format, void *init, int flags);

      /*! load module */
      virtual int loadModule(const char *name);

      /*! assign (named) string parameter to an object */
      virtual void setString(OSPObject object, const char *bufName, const char *s);
      /*! assign (named) data item as a parameter to an object */
      virtual void setObject(OSPObject target, const char *bufName, OSPObject value);
      /*! assign (named) float parameter to an object */
      virtual void setFloat(OSPObject object, const char *bufName, const float f);
      /*! assign (named) vec3f parameter to an object */
      virtual void setVec3f(OSPObject object, const char *bufName, const vec3f &v);
      /*! assign (named) int parameter to an object */
      virtual void setInt(OSPObject object, const char *bufName, const int f);
      /*! assign (named) vec3i parameter to an object */
      virtual void setVec3i(OSPObject object, const char *bufName, const vec3i &v);
      /*! add untyped void pointer to object - this will *ONLY* work in local rendering!  */
      virtual void setVoidPtr(OSPObject object, const char *bufName, void *v) { NOTIMPLEMENTED; }

      /*! create a new triangle mesh geometry */
      virtual OSPTriangleMesh newTriangleMesh();

      /*! create a new renderer object (out of list of registered renderers) */
      virtual OSPRenderer newRenderer(const char *type);

      /*! create a new geometry object (out of list of registered geometrys) */
      virtual OSPGeometry newGeometry(const char *type);

      /*! create a new camera object (out of list of registered cameras) */
      virtual OSPCamera newCamera(const char *type);

      /*! create a new volume object (out of list of registered volumes) */
      virtual OSPVolume newVolume(const char *type);

      /*! call a renderer to render a frame buffer */
      virtual void renderFrame(OSPFrameBuffer _sc, 
                               OSPRenderer _renderer);

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
      virtual void release(OSPObject _obj) {  }

      //! assign given material to given geometry
      virtual void setMaterial(OSPGeometry _geom, OSPMaterial _mat);
      /*! have given renderer create a new material */
      virtual OSPMaterial newMaterial(OSPRenderer _renderer, const char *type);
    };


    
    /*! create this engine, initialize coi with this engine ID, print
      basic device info */
    COIEngine::COIEngine(COIDevice *osprayDevice, size_t engineID)
      : osprayDevice(osprayDevice), engineID(engineID)
    {
      COIRESULT result;
      COI_ISA_TYPE isa = COI_ISA_MIC;      

      cout << "#osp:coi: engine #" << engineID << ": " << flush;
      result = COIEngineGetHandle(isa,engineID,&coiEngine);
      Assert(result == COI_SUCCESS);
      
      result = COIEngineGetInfo(coiEngine,sizeof(coiInfo),&coiInfo);
      Assert(result == COI_SUCCESS);
      
      cout << coiInfo.NumCores << " cores @ " << coiInfo.CoreMaxFrequency << "MHz, "
           << (coiInfo.PhysicalMemory/1000000000) << "GB memory" << endl;
    }

    /*! load ospray worker onto device, and initialize basic ospray
      service */
    void COIEngine::loadOSPRay()
    {
      COIRESULT result;
      result = COIProcessCreateFromFile(coiEngine,
                                        coiWorker,0,NULL,0,NULL,1/*proxy!*/,
                                        NULL,0,NULL,
                                        &coiProcess);
      if (result != COI_SUCCESS)
        coiError(result,"could not load worker binary");
      Assert(result == COI_SUCCESS);
      
      result = COIPipelineCreate(coiProcess,NULL,0,&coiPipe);
      if (result != COI_SUCCESS)
        coiError(result,"could not create command pipe");
      Assert(result == COI_SUCCESS);
      
      struct {
        int32 ID, count;
      } deviceInfo;
      deviceInfo.ID = engineID;
      deviceInfo.count = osprayDevice->engine.size();
      const char *fctName = "ospray_coi_initialize";
      COIFUNCTION fctHandle;
      result = COIProcessGetFunctionHandles(coiProcess,1,
                                            &fctName,&fctHandle);
      if (result != COI_SUCCESS)
        coiError(result,std::string("could not find function '")+fctName+"'");

      result = COIPipelineRunFunction(coiPipe,fctHandle,
                                      0,NULL,NULL,//buffers
                                      0,NULL,//dependencies
                                      &deviceInfo,sizeof(deviceInfo),//data
                                      NULL,0,
                                      NULL);
      Assert(result == COI_SUCCESS);


      result = COIProcessGetFunctionHandles(coiProcess,OSPCOI_NUM_FUNCTIONS,
                                            coiFctName,coiFctHandle);
      if (result != COI_SUCCESS)
        coiError(result,std::string("could not get coi api function handle(s) '"));
      
    }

    void COIEngine::callFunction(RemoteFctID ID, const DataStream &data, bool sync)
    {
      // double t0 = ospray::getSysTime();
      if (sync) {
        bzero(&event,sizeof(event));
        COIRESULT result = COIPipelineRunFunction(coiPipe,coiFctHandle[ID],
                                                  0,NULL,NULL,//buffers
                                                  0,NULL,//dependencies
                                                  data.buf,data.ofs,//data
                                                  NULL,0,
                                                  &event);
        if (result != COI_SUCCESS) {
          coiError(result,"error in coipipelinerunfct");
        }
        COIEventWait(1,&event,-1,1,NULL,NULL);
      } else {
        bzero(&event,sizeof(event));
        COIRESULT result = COIPipelineRunFunction(coiPipe,coiFctHandle[ID],
                                                  0,NULL,NULL,//buffers
                                                  0,NULL,//dependencies
                                                  data.buf,data.ofs,//data
                                                  NULL,0,
                                                  &event);
        if (result != COI_SUCCESS) {
          coiError(result,"error in coipipelinerunfct");
        // COIEventWait(1,&event,-1,1,NULL,NULL);
        }
      }
      // double t1 = ospray::getSysTime();
      // cout << "fct " << ID << "@" << engineID << " : " << (t1-t0) << "s" << endl;
    }

    
    void coiError(COIRESULT result, const std::string &err)
    {
      cerr << "=======================================================" << endl;
      cerr << "#osp:coi(fatal error): " << err << endl;
      if (result != COI_SUCCESS) 
        cerr << "#osp:coi: " << COIResultGetName(result) << endl << flush;
      cerr << "=======================================================" << endl;
      throw std::runtime_error("#osp:coi(fatal error):"+err);
    }
      
    ospray::api::Device *createCoiDevice(int *ac, const char **av)
    {
      cout << "=======================================================" << endl;
      cout << "#osp:mic: trying to create coi device" << endl;
      cout << "=======================================================" << endl;
      COIDevice *device = new COIDevice;
      return device;
    }

    COIDevice::COIDevice()
    {
      COIRESULT result;
      COI_ISA_TYPE isa = COI_ISA_MIC;      

      uint32_t numEngines;
      result = COIEngineGetCount(isa,&numEngines);
      if (numEngines == 0) {
        coiError(result,"no coi devices found");
      }

#if FORCE_SINGLE_DEVICE
      cout << "FORCING AT MOST ONE ENGINE" << endl;
      numEngines = 1;
#endif

      Assert(result == COI_SUCCESS);
      cout << "#osp:coi: found " << numEngines << " COI engines" << endl;
      Assert(numEngines > 0);

      char *maxEnginesFromEnv = getenv("OSPRAY_COI_MAX_ENGINES");
      if (maxEnginesFromEnv) {
        numEngines = max(numEngines,atoi(maxEnginesFromEnv));
        cout << "#osp:coi: max engines after considering 'OSPRAY_COI_MAX_ENGINES' : " << numEngines << endl;
      }


      for (int i=0;i<numEngines;i++)
        engine.push_back(new COIEngine(this,i));

      cout << "#osp:coi: loading ospray onto COI devices..." << endl;
      for (int i=0;i<numEngines;i++)
        engine[i]->loadOSPRay();

      cout << "#osp:coi: all engines initialized and ready to run." << endl;
    }


    int COIDevice::loadModule(const char *name) 
    { 
      cout << "#osp:coi: loading module " << name << endl;
      // cout << "#osp:coi: loading module '" << name << "' not implemented... ignoring" << endl;
      DataStream args;
      args.write(name);

      std::string libName = "libospray_module_"+std::string(name)+"_mic.so";

      COIRESULT result;
      for (int i=0;i<engine.size();i++) {
        COILIBRARY coiLibrary;
        result = COIProcessLoadLibraryFromFile(engine[i]->coiProcess,
                                               libName.c_str(),
                                               NULL,NULL,
                                               // 0,
                                               &coiLibrary);
        if (result != COI_SUCCESS)
          coiError(result,"could not load device library "+libName);
        Assert(result == COI_SUCCESS);
      }
      return 0; 
    }

    /*! create a new triangle mesh geometry */
    OSPTriangleMesh COIDevice::newTriangleMesh()
    {
      Handle ID = Handle::alloc();
      DataStream args;
      args.write(ID);
      callFunction(OSPCOI_NEW_TRIANGLEMESH,args);
      return (OSPTriangleMesh)(int64)ID;
    }


    OSPModel COIDevice::newModel()
    {
      Handle ID = Handle::alloc();
      DataStream args;
      args.write(ID);
      callFunction(OSPCOI_NEW_MODEL,args);
      return (OSPModel)(int64)ID;
    }

    OSPData COIDevice::newData(size_t nitems, OSPDataType format, 
                               void *init, int flags)
    {
      COIRESULT result;
      DataStream args;
      Handle ID = Handle::alloc();

      if (nitems == 0) {
        throw std::runtime_error("cowardly refusing to create empty buffer...");
      }

      args.write(ID);
      args.write((int32)nitems);
      args.write((int32)format);
      args.write((int32)flags);
      // PING;
      // PRINT(nitems);
      // PRINT(format);

      for (int i=0;i<engine.size();i++) {
        COIBUFFER coiBuffer;
        // PRINT(nitems);
        result = COIBufferCreate(nitems*ospray::sizeOf(format),
                                 COI_BUFFER_NORMAL,COI_MAP_READ_WRITE,
                                 init,1,&engine[i]->coiProcess,&coiBuffer);
        Assert(result == COI_SUCCESS);

        COIEVENT event;
        if (init) {
          bzero(&event,sizeof(event));
          result = COIBufferWrite(coiBuffer,//engine[i]->coiProcess,
                                  0,init,nitems*ospray::sizeOf(format),
                         COI_COPY_USE_DMA,0,NULL,&event);
          Assert(result == COI_SUCCESS);
          COIEventWait(1,&event,-1,1,NULL,NULL);
        }

        bzero(&event,sizeof(event));
        COI_ACCESS_FLAGS coiBufferFlags = COI_SINK_READ;
        result = COIPipelineRunFunction(engine[i]->coiPipe,
                                        engine[i]->coiFctHandle[OSPCOI_NEW_DATA],
                                        1,&coiBuffer,&coiBufferFlags,//buffers
                                        0,NULL,//dependencies
                                        args.buf,args.ofs,//data
                                        NULL,0,
                                        &event);
        
        Assert(result == COI_SUCCESS);
        COIEventWait(1,&event,-1,1,NULL,NULL);
        SLEEP_DELAY;
        // usleep(10000000);
      }
      return (OSPData)(int64)ID;
    }

      /*! call a renderer to render a frame buffer */
    void COIDevice::renderFrame(OSPFrameBuffer _sc, 
                               OSPRenderer _renderer)
    {
      DataStream args;
      args.write((Handle&)_sc);
      args.write((Handle&)_renderer);
#if FORCE_SINGLE_DEVICE
      callFunction(OSPCOI_RENDER_FRAME,args,true);
#else
      callFunction(OSPCOI_RENDER_FRAME,args,false);
      callFunction(OSPCOI_RENDER_FRAME_SYNC,args,true);
#endif
   }


    void COIDevice::commit(OSPObject obj)
    {
      Handle handle = (Handle &)obj;
      DataStream args;
      args.write(handle);
      callFunction(OSPCOI_COMMIT,args);
    }

    /*! add a new geometry to a model */
    void COIDevice::addGeometry(OSPModel _model, OSPGeometry _geometry)
    {
      Handle handle = Handle::alloc();
      DataStream args;
      args.write((Handle&)_model);
      args.write((Handle&)_geometry);
      callFunction(OSPCOI_ADD_GEOMETRY,args);
    }


    //! assign given material to given geometry
    void COIDevice::setMaterial(OSPGeometry _geom, OSPMaterial _mat)
    {
      DataStream args;
      args.write((Handle&)_geom);
      args.write((Handle&)_mat);
      callFunction(OSPCOI_SET_MATERIAL,args);
    }

    /*! have given renderer create a new material */
    OSPMaterial COIDevice::newMaterial(OSPRenderer _renderer, const char *type)
    {
      Assert(type);
      Handle handle = Handle::alloc();
      DataStream args;
      args.write(handle);
      args.write(type);
      callFunction(OSPCOI_NEW_MATERIAL,args);
      return (OSPMaterial)(int64)handle;
    }

    /*! create a new geometry object (out of list of registered geometrys) */
    OSPGeometry COIDevice::newGeometry(const char *type)
    {
      Assert(type);
      Handle handle = Handle::alloc();
      DataStream args;
      args.write(handle);
      args.write(type);
      callFunction(OSPCOI_NEW_GEOMETRY,args);
      return (OSPGeometry)(int64)handle;
    }

    /*! create a new camera object (out of list of registered cameras) */
    OSPCamera COIDevice::newCamera(const char *type)
    {
      Assert(type);
      Handle handle = Handle::alloc();
      DataStream args;
      args.write(handle);
      args.write(type);
      callFunction(OSPCOI_NEW_CAMERA,args);
      return (OSPCamera)(int64)handle;
    }

    /*! create a new volume object (out of list of registered volumes) */
    OSPVolume COIDevice::newVolume(const char *type)
    {
      Assert(type);
      Handle handle = Handle::alloc();
      DataStream args;
      args.write(handle);
      args.write(type);
      callFunction(OSPCOI_NEW_VOLUME,args);
      return (OSPVolume)(int64)handle;
    }

    /*! create a new renderer object (out of list of registered renderers) */
    OSPRenderer COIDevice::newRenderer(const char *type)
    {
      Assert(type);
      Handle handle = Handle::alloc();
      DataStream args;
      args.write(handle);
      args.write(type);
      callFunction(OSPCOI_NEW_RENDERER,args);
      return (OSPRenderer)(int64)handle;
    }



    struct COIFrameBuffer {
      COIBUFFER *coiBuffer; // one per engine
      void *hostMem;
      vec2i size;
    };

    std::map<int64,COIFrameBuffer *> fbList;

      /*! create a new frame buffer */
    OSPFrameBuffer COIDevice::frameBufferCreate(const vec2i &size, 
                                                const OSPFrameBufferMode mode,
                                                const size_t swapChainDepth)
    {
      COIRESULT result;
      Handle handle = Handle::alloc();
      DataStream args;
      args.write(handle);
      args.write(size);

      Assert(mode == OSP_RGBA_I8);
      COIFrameBuffer *fb = new COIFrameBuffer;
      fbList[handle] = fb;
      fb->hostMem = new int32[size.x*size.y];
      fb->coiBuffer = new COIBUFFER[engine.size()];
      fb->size = size;
      for (int i=0;i<engine.size();i++) {
        result = COIBufferCreate(size.x*size.y*sizeof(int32),
                                 COI_BUFFER_NORMAL,COI_MAP_READ_WRITE,
                                 NULL,1,&engine[i]->coiProcess,&fb->coiBuffer[i]);
        Assert(result == COI_SUCCESS);
        
        COIEVENT event; bzero(&event,sizeof(event));
        COI_ACCESS_FLAGS coiBufferFlags = 
          (COI_ACCESS_FLAGS)((int)COI_SINK_READ 
                             | (int)COI_SINK_WRITE);
        result = COIPipelineRunFunction(engine[i]->coiPipe,
                                        engine[i]->coiFctHandle[OSPCOI_NEW_FRAMEBUFFER],
                                        1,&fb->coiBuffer[i],
                                        &coiBufferFlags,//buffers
                                        0,NULL,//dependencies
                                        args.buf,args.ofs,//data
                                        NULL,0,
                                        &event);
        Assert(result == COI_SUCCESS);
        COIEventWait(1,&event,-1,1,NULL,NULL);
        SLEEP_DELAY;
      }
      return (OSPFrameBuffer)(int64)handle;
    }

      /*! map frame buffer */
    const void *COIDevice::frameBufferMap(OSPFrameBuffer _fb)
    {
      COIRESULT result;
      Handle handle = (Handle &)_fb;
      COIFrameBuffer *fb = fbList[handle];//(COIFrameBuffer *)_fb;
      
#if FORCE_SINGLE_DEVICE
      COIEVENT event; bzero(&event,sizeof(event));
      // double t0 = ospray::getSysTime();
      result = COIBufferRead(fb->coiBuffer[0],0,fb->hostMem,
                             fb->size.x*fb->size.y*sizeof(int32),
                             COI_COPY_USE_DMA,0,NULL,&event);
      Assert(result == COI_SUCCESS);
      COIEventWait(1,&event,-1,1,NULL,NULL);
      // double t1 = ospray::getSysTime();
      // double t_read_buffer = t1 - t0;
      // PRINT(t_read_buffer);
#else
      const int numEngines = engine.size();
      int32 *devBuffer[numEngines];
      COIEVENT doneCopy[numEngines];
      // -------------------------------------------------------
      // trigger N copies...
      // -------------------------------------------------------
      for (int i=0;i<numEngines;i++) {
        bzero(&doneCopy[i],sizeof(COIEVENT));
        devBuffer[i] = new int32[fb->size.x*fb->size.y];
        result = COIBufferRead(fb->coiBuffer[i],0,devBuffer[i],
                               fb->size.x*fb->size.y*sizeof(int32),
                               COI_COPY_USE_DMA,0,NULL,&doneCopy[i]);
        Assert(result == COI_SUCCESS);
      }
      // -------------------------------------------------------
      // do 50 assemblies...
      // -------------------------------------------------------
      for (int engineID=0;engineID<numEngines;engineID++) {
        const size_t sizeX = fb->size.x;
        const size_t sizeY = fb->size.y;
        COIEventWait(1,&doneCopy[engineID],-1,1,NULL,NULL);
        uint32 *src = (uint32*)devBuffer[engineID];
        uint32 *dst = (uint32*)fb->hostMem;

        const size_t numTilesX = divRoundUp(sizeX,TILE_SIZE);
        const size_t numTilesY = divRoundUp(sizeY,TILE_SIZE);
        for (size_t tileY=0;tileY<numTilesY;tileY++)
          for (size_t tileX=0;tileX<numTilesX;tileX++) {
            const size_t tileID = tileX+numTilesX*tileY;
            if (engineID != (tileID % numEngines)) 
              continue;
            const size_t x0 = tileX*TILE_SIZE;            
            const size_t x1 = std::min(x0+TILE_SIZE,sizeX);
            const size_t y0 = tileY*TILE_SIZE;            
            const size_t y1 = std::min(y0+TILE_SIZE,sizeY);
            for (size_t y=y0;y<y1;y++)
              for (size_t x=x0;x<x1;x++) {
                const size_t idx = x+y*sizeX;
                dst[idx] = src[idx];
              }
          }

        delete[] devBuffer[engineID];
      }
#endif
      return fb->hostMem;
    }

      /*! unmap previously mapped frame buffer */
    void COIDevice::frameBufferUnmap(const void *mapped,
                                     OSPFrameBuffer fb)
    {
      // PING; // nothing to do!?
    }



    /*! assign (named) data item as a parameter to an object */
    void COIDevice::setObject(OSPObject target, const char *bufName, OSPObject value)
    {
      Assert(bufName);

      DataStream args;
      args.write((Handle&)target);
      args.write(bufName);
      args.write(OSP_OBJECT);
      args.write((Handle&)value);
      callFunction(OSPCOI_SET_VALUE,args);
    }

    /*! assign (named) data item as a parameter to an object */
    void COIDevice::setString(OSPObject target, const char *bufName, const char *s)
    {
      Assert(bufName);

      DataStream args;
      args.write((Handle&)target);
      args.write(bufName);
      args.write(OSP_STRING);
      args.write(s);
      callFunction(OSPCOI_SET_VALUE,args);
    }
    /*! assign (named) data item as a parameter to an object */
    void COIDevice::setFloat(OSPObject target, const char *bufName, const float f)
    {
      Assert(bufName);

      DataStream args;
      args.write((Handle&)target);
      args.write(bufName);
      args.write(OSP_float);
      args.write(f);
      callFunction(OSPCOI_SET_VALUE,args);
    }
    /*! assign (named) data item as a parameter to an object */
    void COIDevice::setInt(OSPObject target, const char *bufName, const int32 i)
    {
      Assert(bufName);

      DataStream args;
      args.write((Handle&)target);
      args.write(bufName);
      args.write(OSP_int32);
      args.write(i);
      callFunction(OSPCOI_SET_VALUE,args);
    }
    /*! assign (named) data item as a parameter to an object */
    void COIDevice::setVec3f(OSPObject target, const char *bufName, const vec3f &v)
    {
      Assert(bufName);

      DataStream args;
      args.write((Handle&)target);
      args.write(bufName);
      args.write(OSP_vec3f);
      args.write(v);
      callFunction(OSPCOI_SET_VALUE,args);
    }
    /*! assign (named) data item as a parameter to an object */
    void COIDevice::setVec3i(OSPObject target, const char *bufName, const vec3i &v)
    {
      Assert(bufName);

      DataStream args;
      args.write((Handle&)target);
      args.write(bufName);
      args.write(OSP_vec3i);
      args.write(v);
      callFunction(OSPCOI_SET_VALUE,args);
    }
  }
}


