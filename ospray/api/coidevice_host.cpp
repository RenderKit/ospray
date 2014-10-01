/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#undef NDEBUG

// OSPRay
#include "device.h"
#include "coidevice_common.h"
#include "ospray/common/data.h"
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


#define MAX_ENGINES 100

#define MANUAL_DATA_UPLOADS 1
#define UPLOAD_BUFFER_SIZE (2LL*1024*1024)

namespace ospray {
  namespace coi {

    // const char *coiWorker = "./ospray_coi_worker.mic";

    void coiError(COIRESULT result, const std::string &err);

    struct COIDevice;

#define OSPCOI_FUNCTION_MAPPING                                             \
      x(OSPCOI_NEW_MODEL=0,             "ospray_coi_new_model")             \
      x(OSPCOI_NEW_DATA,                "ospray_coi_new_data")              \
      x(OSPCOI_NEW_TRIANGLEMESH,        "ospray_coi_new_trianglemesh")      \
      x(OSPCOI_COMMIT,                  "ospray_coi_commit")                \
      x(OSPCOI_SET_VALUE,               "ospray_coi_set_value")             \
      x(OSPCOI_NEW_MATERIAL,            "ospray_coi_new_material")          \
      x(OSPCOI_SET_MATERIAL,            "ospray_coi_set_material")          \
      x(OSPCOI_NEW_CAMERA,              "ospray_coi_new_camera")            \
      x(OSPCOI_NEW_VOLUME,              "ospray_coi_new_volume")            \
      x(OSPCOI_NEW_VOLUME_FROM_FILE,    "ospray_coi_new_volume_from_file")  \
      x(OSPCOI_NEW_TRANSFER_FUNCTION,   "ospray_coi_new_transfer_function") \
      x(OSPCOI_NEW_RENDERER,            "ospray_coi_new_renderer")          \
      x(OSPCOI_NEW_GEOMETRY,            "ospray_coi_new_geometry")          \
      x(OSPCOI_ADD_GEOMETRY,            "ospray_coi_add_geometry")          \
      x(OSPCOI_NEW_FRAMEBUFFER,         "ospray_coi_new_framebuffer")       \
      x(OSPCOI_RENDER_FRAME,            "ospray_coi_render_frame")          \
      x(OSPCOI_RENDER_FRAME_SYNC,       "ospray_coi_render_frame_sync")     \
      x(OSPCOI_NEW_TEXTURE2D,           "ospray_coi_new_texture2d")         \
      x(OSPCOI_NEW_LIGHT,               "ospray_coi_new_light")             \
      x(OSPCOI_REMOVE_GEOMETRY,         "ospray_coi_remove_geometry")       \
      x(OSPCOI_FRAMEBUFFER_CLEAR,       "ospray_coi_framebuffer_clear")     \
      x(OSPCOI_PIN_UPLOAD_BUFFER,       "ospray_coi_pin_upload_buffer")     \
      x(OSPCOI_CREATE_NEW_EMPTY_DATA,   "ospray_coi_create_new_empty_data") \
      x(OSPCOI_UPLOAD_DATA_DONE,        "ospray_coi_upload_data_done")      \
      x(OSPCOI_UPLOAD_DATA_CHUNK,       "ospray_coi_upload_data_chunk")     \
      x(OSPCOI_UNPROJECT,               "ospray_coi_unproject")             \
      x(OSPCOI_NUM_FUNCTIONS,           NULL) //This must be last

#define x(a,b) a,
    typedef enum {
      OSPCOI_FUNCTION_MAPPING
    } RemoteFctID;
#undef x

#define x(a,b) b,
    const  char *coiFctName[] = { OSPCOI_FUNCTION_MAPPING };
#undef x

    struct COIEngine {
#ifdef MANUAL_DATA_UPLOADS
      COIBUFFER uploadBuffer;
#endif
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
      // void callFunction(RemoteFctID ID, const DataStream &data, int *returnValue, bool sync);
    };

    struct COIDevice : public ospray::api::Device {
      std::vector<COIEngine *> engine;
      api::Handle handle;

      COIDevice();

      void callFunction(RemoteFctID ID, const DataStream &data, 
                        void *returnValue=NULL, 
                        int returnValueSize=0,
                        bool sync=true)
      { 
        double t0 = getSysTime();
        // cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << endl;
        // cout << "calling coi function " << coiFctName[ID] << endl;
        // cout << "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$" << endl;
#if 1
        static COIEVENT event[MAX_ENGINES]; //at most 100 engines...
        static long numEventsOutstanding = 0;
        assert(engine.size() < MAX_ENGINES);
        for (int i=0;i<engine.size();i++) {
          if (numEventsOutstanding == 0) {
            bzero(&event[i],sizeof(event[i]));
          }
          if (ospray::logLevel > 0 || ospray::debugMode) 
            std::cout << "#osp:coi: calling coi fct " << coiFctName[ID] << std::endl;
          COIRESULT result = COIPipelineRunFunction(engine[i]->coiPipe,
                                                    engine[i]->coiFctHandle[ID],
                                                    0,NULL,NULL,//buffers
                                                    0,NULL,//dependencies
                                                    data.buf,data.ofs,//data
                                                    returnValue,
                                                    returnValue?returnValueSize:0,
                                                    &event[i]);
          if (result != COI_SUCCESS) {
            coiError(result,"error in coipipelinerunfct");
          }
        }
        numEventsOutstanding++;
        if (sync || returnValue) {
          for (int i=0;i<engine.size();i++) {
            COIEventWait(1,&event[i],-1,1/*wait for all*/,NULL,NULL);
          }
          numEventsOutstanding = 0;
        }
#else
        for (int i=0;i<engine.size();i++) engine[i]->callFunction(ID,data,sync); 
#endif
      }

      /*! create a new frame buffer */
      virtual OSPFrameBuffer frameBufferCreate(const vec2i &size, 
                                               const OSPFrameBufferFormat mode,
                                               const uint32 channels);


      /*! clear the specified channel(s) of the frame buffer specified in 'whichChannels'
        
        if whichChannel&OSP_FB_COLOR!=0, clear the color buffer to
        '0,0,0,0'.  

        if whichChannel&OSP_FB_DEPTH!=0, clear the depth buffer to
        +inf.  

        if whichChannel&OSP_FB_ACCUM!=0, clear the accum buffer to 0,0,0,0,
        and reset accumID.
      */
      virtual void frameBufferClear(OSPFrameBuffer _fb,
                                    const uint32 fbChannelFlags); 

      /*! map frame buffer */
      virtual const void *frameBufferMap(OSPFrameBuffer fb, 
                                         OSPFrameBufferChannel);

      /*! unmap previously mapped frame buffer */
      virtual void frameBufferUnmap(const void *mapped,
                                    OSPFrameBuffer fb);

      /*! create a new model */
      virtual OSPModel newModel();

      // /*! finalize a newly specified model */
      // virtual void finalizeModel(OSPModel _model) { NOTIMPLEMENTED; }

      /*! commit the given object's outstanding changes */
      virtual void commit(OSPObject object);
      /*! remove an existing geometry from a model */
      virtual void removeGeometry(OSPModel _model, OSPGeometry _geometry);

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

      /*! create a new transfer function object (out of list of registered transfer function types) */
      virtual OSPTransferFunction newTransferFunction(const char *type);

      /*! have given renderer create a new Light */
      virtual OSPLight newLight(OSPRenderer _renderer, const char *type);

      /*! create a new Texture2D object */
      virtual OSPTexture2D newTexture2D(int width, int height, OSPDataType type, void *data, int flags);
      
      /*! call a renderer to render a frame buffer */
      virtual void renderFrame(OSPFrameBuffer _sc, 
                               OSPRenderer _renderer, 
                               const uint32 fbChannelFlags);

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

      virtual OSPPickData unproject(OSPRenderer _renderer, const vec2f &screenPos);
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
      const char *coiWorker = getenv("OSPRAY_COI_WORKER");
      if (coiWorker == NULL) {
        std::cerr << "Error: OSPRAY_COI_WORKER not defined." << std::endl;
        std::cerr << "Note: In order to run the OSPRay COI device on the Xeon Phi(s) it needs to know the full path of the 'ospray_coi_worker.mic' executable that contains the respective ospray xeon phi worker binary. Please define an environment variable named 'OSPRAY_COI_WORKER' to contain the filename - with full directory path - of this executable." << std::endl;
        exit(1);
      }
      const char *sinkLDPath = getenv("SINK_LD_LIBRARY_PATH");
      if (coiWorker == NULL) {
        std::cerr << "SINK_LD_LIBRARY_PATH not defined." << std::endl;
        std::cerr << "Note: In order for the COI version of OSPRay to find all the shared libraries (ospray, plus whatever modules the application way want to load) you have to specify the search path where COI is supposed to find those libraries on the HOST filesystem (it will then load them onto the device as required)." << std::endl;
        std::cerr << "Please define an environment variable named SINK_LD_LIBRARY_PATH that points to the directory containing the respective ospray mic libraries." << std::endl;
        exit(1);
      }

      std::vector<const char*> workerArgs;
      workerArgs.push_back(coiWorker);
      if (ospray::debugMode)
        workerArgs.push_back("--osp:debug");
      if (ospray::logLevel == 1)
        workerArgs.push_back("--osp:verbose");
      if (ospray::logLevel == 2)
        workerArgs.push_back("--osp:vv");

      result = COIProcessCreateFromFile(coiEngine,
                                        coiWorker,
                                        workerArgs.size(),
                                        &workerArgs[0],
                                        0,NULL,1/*proxy!*/,
                                        NULL,0,NULL,
                                        &coiProcess);
      // result = COIProcessCreateFromFile(coiEngine,
      //                                   coiWorker,0,NULL,0,NULL,1/*proxy!*/,
      //                                   NULL,0,NULL,
      //                                   &coiProcess);
      if (result != COI_SUCCESS)
        coiError(result,"could not load worker binary");
      Assert(result == COI_SUCCESS);
      
      result = COIPipelineCreate(coiProcess,NULL,0,&coiPipe);
      if (result != COI_SUCCESS)
        coiError(result,"could not create command pipe");
      Assert(result == COI_SUCCESS);
      

      struct {
        int32 ID, count, debugMode, logLevel;
      } deviceInfo;
      deviceInfo.ID = engineID;
      deviceInfo.count = osprayDevice->engine.size();
      deviceInfo.debugMode = ospray::debugMode;
      deviceInfo.logLevel = ospray::logLevel;
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
      

#ifdef MANUAL_DATA_UPLOADS
      size_t size = UPLOAD_BUFFER_SIZE;
      char *uploadBufferMem = new char[UPLOAD_BUFFER_SIZE];
      result = COIBufferCreate(size,COI_BUFFER_NORMAL,0,
                               uploadBufferMem,1,&coiProcess,&uploadBuffer);
      if (result != COI_SUCCESS) {
        cout << "error in allocating coi buffer : " << COIResultGetName(result) << endl;
        FATAL("error in allocating coi buffer");
      }
      {
        COI_ACCESS_FLAGS coiBufferFlags = COI_SINK_READ;
        DataStream args;
        COIEVENT event;
        args.write((int)1);
        result = COIPipelineRunFunction(coiPipe,
                                        coiFctHandle[OSPCOI_PIN_UPLOAD_BUFFER],
                                        1,&uploadBuffer,&coiBufferFlags,//buffers
                                        0,NULL,//dependencies
                                        args.buf,args.ofs,//data
                                        NULL,0,
                                        &event);
        COIEventWait(1,&event,-1,1,NULL,NULL);
        if (result != COI_SUCCESS) {
          cout << "error in pinning coi upload buffer : " << COIResultGetName(result) << endl;
          FATAL("error in allocating coi buffer");
        }
      }
#endif
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

      Assert(result == COI_SUCCESS);
      cout << "#osp:coi: found " << numEngines << " COI engines" << endl;
      Assert(numEngines > 0);

      char *maxEnginesFromEnv = getenv("OSPRAY_COI_MAX_ENGINES");
      if (maxEnginesFromEnv) {
        numEngines = std::min((int)numEngines,(int)atoi(maxEnginesFromEnv));
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

      double t0 = getSysTime();
      COIEVENT event[engine.size()];
      COIBUFFER coiBuffer[engine.size()];

      size_t size = nitems*ospray::sizeOf(format);

#if MANUAL_DATA_UPLOADS
      callFunction(OSPCOI_CREATE_NEW_EMPTY_DATA,args);
      for (size_t begin=0;begin<size;begin+=UPLOAD_BUFFER_SIZE) {
        size_t blockSize = std::min((ulong)UPLOAD_BUFFER_SIZE,(ulong)(size-begin));
        char *beginPtr = ((char*)init)+begin;
        for (int i=0;i<engine.size();i++) {
          COIEVENT event;
          result = COIBufferWrite(engine[i]->uploadBuffer,
                                  0,beginPtr,blockSize,
                                  COI_COPY_USE_DMA,
                                  0,NULL,&event);
          if (result != COI_SUCCESS)
            cout << "error in allocating coi buffer : " << COIResultGetName(result) << endl;
          COIEventWait(1,&event,-1,1,NULL,NULL);
        }
        
        DataStream args;
        args.write(ID);
        args.write((int64)begin);
        args.write((int64)blockSize);


        for (int i=0;i<engine.size();i++) {
          COIEVENT event;
          bzero(&event,sizeof(event));
          COI_ACCESS_FLAGS coiBufferFlags = COI_SINK_READ;
          result = COIPipelineRunFunction(engine[i]->coiPipe,
                                          engine[i]->coiFctHandle[OSPCOI_UPLOAD_DATA_CHUNK],
                                          1,&engine[i]->uploadBuffer,&coiBufferFlags,//buffers
                                          0,NULL,//dependencies
                                          args.buf,args.ofs,//data
                                          NULL,0,
                                          NULL); //&event);
          if (result != COI_SUCCESS)
            cout << "error in allocating coi buffer : " << COIResultGetName(result) << endl;
        }        
        Assert(result == COI_SUCCESS);
      }
      callFunction(OSPCOI_UPLOAD_DATA_DONE,args);
#else
      for (int i=0;i<engine.size();i++) {
        size_t size = nitems*ospray::sizeOf(format);

        result = COIBufferCreate(size,COI_BUFFER_NORMAL,
                                 size>128*1024*1024?COI_OPTIMIZE_HUGE_PAGE_SIZE:0,
                                 init,1,&engine[i]->coiProcess,&coiBuffer[i]);
        if (result != COI_SUCCESS) {
          cout << "error in allocating coi buffer : " << COIResultGetName(result) << endl;
          FATAL("error in allocating coi buffer");
        }

        {
          DataStream args;
          int i = 0;
          args.write(i);
          bzero(&event[i],sizeof(event[i]));
          COI_ACCESS_FLAGS coiBufferFlags = COI_SINK_READ;
          result = COIPipelineRunFunction(engine[i]->coiPipe,
                                          engine[i]->coiFctHandle[OSPCOI_PRINT_CHECKSUMS],
                                          1,&coiBuffer[i],&coiBufferFlags,//buffers
                                          0,NULL,//dependencies
                                          args.buf,args.ofs,//data
                                          NULL,0,
                                          &event[i]);
          COIEventWait(1,&event[i],-1,1,NULL,NULL);
        }

        Assert(result == COI_SUCCESS);

      }

      for (int i=0;i<engine.size();i++) {
        cout << "callling osp_coi_new_data" << endl;
        bzero(&event[i],sizeof(event[i]));
        COI_ACCESS_FLAGS coiBufferFlags = COI_SINK_READ;
        result = COIPipelineRunFunction(engine[i]->coiPipe,
                                        engine[i]->coiFctHandle[OSPCOI_NEW_DATA],
                                        1,&coiBuffer[i],&coiBufferFlags,//buffers
                                        0,NULL,//dependencies
                                        args.buf,args.ofs,//data
                                        NULL,0,
                                        &event[i]);
        
        Assert(result == COI_SUCCESS);
      }
      for (int i=0;i<engine.size();i++) {
        COIEventWait(1,&event[i],-1,1,NULL,NULL);
      }
#endif
      return (OSPData)(int64)ID;
    }

    /*! call a renderer to render a frame buffer */
    void COIDevice::renderFrame(OSPFrameBuffer _sc, 
                                OSPRenderer _renderer, 
                                const uint32 fbChannelFlags)
    {
      DataStream args;
      args.write((Handle&)_sc);
      args.write((Handle&)_renderer);
      args.write((uint32&)fbChannelFlags);
      callFunction(OSPCOI_RENDER_FRAME,args,NULL,false);
      callFunction(OSPCOI_RENDER_FRAME_SYNC,args,NULL,true);
    }


    void COIDevice::commit(OSPObject obj)
    {
      Handle handle = (Handle &)obj;
      DataStream args;
      args.write(handle);
      callFunction(OSPCOI_COMMIT,args);
    }

    void COIDevice::removeGeometry(OSPModel _model, OSPGeometry _geometry)
    {
      DataStream args;
      args.write((Handle&)_model);
      args.write((Handle&)_geometry);
      callFunction(OSPCOI_REMOVE_GEOMETRY,args);
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
      args.write(_renderer);
      args.write(type);
      int retValue = -1;
      callFunction(OSPCOI_NEW_MATERIAL,args,&retValue,sizeof(int));
      if (retValue)
        // could create material ...
        return (OSPMaterial)(int64)handle;
      else {
        // could NOT create materail 
        handle.free();
        return (OSPMaterial)NULL;
      }
    }

    /*! have given renderer unproject a screenspace point to worldspace */
    OSPPickData COIDevice::unproject(OSPRenderer _renderer, const vec2f &screenPos)
    {
      Assert2(_renderer, "NULL renderer in COIDevice::unproject");
      DataStream args;
      args.write(_renderer);
      args.write(screenPos);

      OSPPickData retValue = {false, 0, 0, 0};
      callFunction(OSPCOI_UNPROJECT, args, &retValue, sizeof(retValue));
      return retValue;
    }

    /*! create a new texture2D */
    OSPTexture2D COIDevice::newTexture2D(int width, int height, OSPDataType type, void *data, int flags)
    {
      COIRESULT result;
      DataStream args;
      Handle ID = Handle::alloc();

      if (width * height == 0) {
        throw std::runtime_error("cowardly refusing to create empty texture...");
      }

      args.write(ID);
      args.write((int32)width);
      args.write((int32)height);
      args.write((int32)type);
      args.write((int32)flags);
      int64 numBytes = sizeOf(type)*width*height;
      // double t0 = getSysTime();
      for (int i=0;i<engine.size();i++) {
        COIBUFFER coiBuffer;
        // PRINT(nitems);
        result = COIBufferCreate(numBytes,COI_BUFFER_NORMAL,COI_OPTIMIZE_HUGE_PAGE_SIZE,//COI_MAP_READ_ONLY,
                                 data,1,&engine[i]->coiProcess,&coiBuffer);
        Assert(result == COI_SUCCESS);
        COIEVENT event;
        bzero(&event,sizeof(event));
        COI_ACCESS_FLAGS coiBufferFlags = COI_SINK_READ;
        result = COIPipelineRunFunction(engine[i]->coiPipe,
                                        engine[i]->coiFctHandle[OSPCOI_NEW_TEXTURE2D],
                                        1,&coiBuffer,&coiBufferFlags,//buffers
                                        0,NULL,//dependencies
                                        args.buf,args.ofs,//data
                                        NULL,0,
                                        &event);
        Assert(result == COI_SUCCESS);
        COIEventWait(1,&event,-1,1,NULL,NULL);
      }
      // double t1 = getSysTime();
      // static double sum_t = 0;
      // sum_t += (t1-t0);
      //      cout << "time spent in createtexture2d " << (t1-t0) << " total " << sum_t << endl;
      return (OSPTexture2D)(int64)ID;
    }

    /*! have given renderer create a new light */
    OSPLight COIDevice::newLight(OSPRenderer _renderer, const char *type)
    {
      Assert(type);
      Handle handle = Handle::alloc();
      DataStream args;
      
      args.write(handle);
      args.write((Handle&)_renderer);
      args.write(type);
      callFunction(OSPCOI_NEW_LIGHT,args);
      return (OSPLight)(int64)handle;
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

    /*! create a new transfer function object (out of list of registered transfer function types) */
    OSPTransferFunction COIDevice::newTransferFunction(const char *type)
    {
      Assert(type);
      Handle handle = Handle::alloc();
      DataStream args;
      args.write(handle);
      args.write(type);
      callFunction(OSPCOI_NEW_TRANSFER_FUNCTION, args);
      return (OSPTransferFunction)(int64) handle;
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
                                                const OSPFrameBufferFormat mode,
                                                const uint32 channels)
    {
      COIRESULT result;
      Handle handle = Handle::alloc();
      DataStream args;
      args.write(handle);
      args.write(size);
      args.write((uint32)mode);
      args.write(channels);

      Assert(mode == OSP_RGBA_I8);
      COIFrameBuffer *fb = new COIFrameBuffer;
      fbList[handle] = fb;
      fb->hostMem = new int32[size.x*size.y];
      fb->coiBuffer = new COIBUFFER[engine.size()];
      fb->size = size;
      for (int i=0;i<engine.size();i++) {
        result = COIBufferCreate(size.x*size.y*sizeof(int32),
                                 COI_BUFFER_NORMAL,COI_OPTIMIZE_HUGE_PAGE_SIZE,//COI_MAP_READ_WRITE,
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
      }
      return (OSPFrameBuffer)(int64)handle;
    }

    /*! map frame buffer */
    const void *COIDevice::frameBufferMap(OSPFrameBuffer _fb,
                                          OSPFrameBufferChannel channel)
    {
      if (channel != OSP_FB_COLOR)
        throw std::runtime_error("can only map color buffers on coi devices");
      COIRESULT result;
      Handle handle = (Handle &)_fb;
      COIFrameBuffer *fb = fbList[handle];//(COIFrameBuffer *)_fb;
      
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
// #pragma omp parallel for
        for (size_t tileY=0;tileY<numTilesY;tileY++) {
// #pragma omp parallel for
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
        }

        delete[] devBuffer[engineID];
      }
      return fb->hostMem;
    }

    /*! unmap previously mapped frame buffer */
    void COIDevice::frameBufferUnmap(const void *mapped,
                                     OSPFrameBuffer fb)
    {
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
      args.write(OSP_FLOAT);
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
      args.write(OSP_INT);
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
      args.write(OSP_FLOAT3);
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
      args.write(OSP_INT3);
      args.write(v);
      callFunction(OSPCOI_SET_VALUE,args);
    }
    /*! clear the specified channel(s) of the frame buffer specified in 'whichChannels'
        
      if whichChannel&OSP_FB_COLOR!=0, clear the color buffer to
      '0,0,0,0'.  

      if whichChannel&OSP_FB_DEPTH!=0, clear the depth buffer to
      +inf.  

      if whichChannel&OSP_FB_ACCUM!=0, clear the accum buffer to 0,0,0,0,
      and reset accumID.
    */
    void COIDevice::frameBufferClear(OSPFrameBuffer _fb,
                                     const uint32 fbChannelFlags)
    {
      DataStream args;
      args.write((Handle&)_fb);
      args.write(fbChannelFlags);
      callFunction(OSPCOI_FRAMEBUFFER_CLEAR,args);
    }
  }
}


