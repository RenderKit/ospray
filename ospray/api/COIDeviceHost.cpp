// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#undef NDEBUG

// ospray
#include "Device.h"
#include "COIDeviceCommon.h"
#include "ospray/common/Data.h"
// coi
#include "common/COIResult_common.h"
#include "source/COIEngine_source.h"
#include "source/COIEvent_source.h"
#include "source/COIProcess_source.h"
#include "source/COIBuffer_source.h"
#include "source/COIPipeline_source.h"
// std
#include <map>

#include "ospray/fb/tileSize.h"

#define MAX_ENGINES 100

#define MANUAL_DATA_UPLOADS 1
#define UPLOAD_BUFFER_SIZE (2LL*1024*1024)

namespace ospray {
  namespace coi {

    void coiError(COIRESULT result, const std::string &err);

    struct COIDevice;

#define OSPCOI_FUNCTION_MAPPING                                             \
      x(OSPCOI_NEW_MODEL=0,             "ospray_coi_new_model")             \
      x(OSPCOI_NEW_DATA,                "ospray_coi_new_data")              \
      x(OSPCOI_NEW_TRIANGLEMESH,        "ospray_coi_new_trianglemesh")      \
      x(OSPCOI_COMMIT,                  "ospray_coi_commit")                \
      x(OSPCOI_SET_VALUE,               "ospray_coi_set_value")             \
      x(OSPCOI_GET_DATA_PROPERTIES,     "ospray_coi_get_data_properties")   \
      x(OSPCOI_GET_DATA_VALUES,         "ospray_coi_get_data_values")       \
      x(OSPCOI_GET_PARAMETERS,          "ospray_coi_get_parameters")        \
      x(OSPCOI_GET_PARAMETERS_SIZE,     "ospray_coi_get_parameters_size")   \
      x(OSPCOI_GET_TYPE,                "ospray_coi_get_type")              \
      x(OSPCOI_GET_VALUE,               "ospray_coi_get_value")             \
      x(OSPCOI_NEW_MATERIAL,            "ospray_coi_new_material")          \
      x(OSPCOI_SET_MATERIAL,            "ospray_coi_set_material")          \
      x(OSPCOI_NEW_CAMERA,              "ospray_coi_new_camera")            \
      x(OSPCOI_NEW_VOLUME,              "ospray_coi_new_volume")            \
      x(OSPCOI_SET_REGION,              "ospray_coi_set_region")            \
      x(OSPCOI_NEW_TRANSFER_FUNCTION,   "ospray_coi_new_transfer_function") \
      x(OSPCOI_NEW_RENDERER,            "ospray_coi_new_renderer")          \
      x(OSPCOI_NEW_GEOMETRY,            "ospray_coi_new_geometry")          \
      x(OSPCOI_ADD_GEOMETRY,            "ospray_coi_add_geometry")          \
      x(OSPCOI_ADD_VOLUME,              "ospray_coi_add_volume")            \
      x(OSPCOI_NEW_FRAMEBUFFER,         "ospray_coi_new_framebuffer")       \
      x(OSPCOI_RENDER_FRAME,            "ospray_coi_render_frame")          \
      x(OSPCOI_RENDER_FRAME_SYNC,       "ospray_coi_render_frame_sync")     \
      x(OSPCOI_NEW_TEXTURE2D,           "ospray_coi_new_texture2d")         \
      x(OSPCOI_NEW_LIGHT,               "ospray_coi_new_light")             \
      x(OSPCOI_RELEASE,                 "ospray_coi_release")               \
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

      /*! add a new volume to a model */
      virtual void addVolume(OSPModel _model, OSPVolume _volume);

      /*! create a new data buffer */
      virtual OSPData newData(size_t nitems, OSPDataType format, void *init, int flags);

      /*! load module */
      virtual int loadModule(const char *name);

      /*! Copy data into the given volume. */
      virtual int setRegion(OSPVolume object, const void *source, 
                            const vec3i &index, const vec3i &count);

      /*! assign (named) string parameter to an object */
      virtual void setString(OSPObject object, const char *bufName, const char *s);

      /*! assign (named) data item as a parameter to an object */
      virtual void setObject(OSPObject target, const char *bufName, OSPObject value);

      /*! assign (named) float parameter to an object */
      virtual void setFloat(OSPObject object, const char *bufName, const float f);

      /*! assign (named) vec2f parameter to an object */
      virtual void setVec2f(OSPObject object, const char *bufName, const vec2f &v);

      /*! assign (named) vec3f parameter to an object */
      virtual void setVec3f(OSPObject object, const char *bufName, const vec3f &v);

      /*! assign (named) int parameter to an object */
      virtual void setInt(OSPObject object, const char *bufName, const int f);

      /*! assign (named) vec3i parameter to an object */
      virtual void setVec3i(OSPObject object, const char *bufName, const vec3i &v);

      /*! add untyped void pointer to object - this will *ONLY* work in local rendering!  */
      virtual void setVoidPtr(OSPObject object, const char *bufName, void *v) { NOTIMPLEMENTED; }

      /*! Get the handle of the named data array associated with an object. */
      virtual int getData(OSPObject object, const char *name, OSPData *value);

      /*! Get the type and count of the elements contained in the given array object.*/
      int getDataProperties(OSPData object, size_t *count, OSPDataType *type);

      /*! Get a copy of the data in an array (the application is responsible for freeing this pointer). */
      virtual int getDataValues(OSPData object, void **pointer, size_t *count, OSPDataType *type);

      /*! Get the named scalar floating point value associated with an object. */
      virtual int getf(OSPObject object, const char *name, float *value);

      /*! Get the named scalar integer associated with an object. */
      virtual int geti(OSPObject object, const char *name, int *value);

      /*! Get the material associated with a geometry object. */
      virtual int getMaterial(OSPGeometry geometry, OSPMaterial *value);

      /*! Get the named object associated with an object. */
      virtual int getObject(OSPObject object, const char *name, OSPObject *value);

      /*! Retrieve a NULL-terminated list of the parameter names associated with an object. */
      virtual int getParameters(OSPObject object, char ***value);

      /*! Retrieve the total length of the names (with terminators) of the parameters associated with an object. */
      int getParametersSize(OSPObject object, int *value);

      /*! Get a pointer to a copy of the named character string associated with an object. */
      virtual int getString(OSPObject object, const char *name, char **value);

      /*! Get the type of the named parameter or the given object (if 'name' is NULL). */
      virtual int getType(OSPObject object, const char *name, OSPDataType *value);

      /*! Get the named 2-vector floating point value associated with an object. */
      virtual int getVec2f(OSPObject object, const char *name, vec2f *value);

      /*! Get the named 3-vector floating point value associated with an object. */
      virtual int getVec3f(OSPObject object, const char *name, vec3f *value);

      /*! Get the named 3-vector integer value associated with an object. */
      virtual int getVec3i(OSPObject object, const char *name, vec3i *value);

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
      virtual void release(OSPObject _obj);

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

    void COIDevice::release(OSPObject object)
    {
      if (object == NULL) return;
      Handle handle = (Handle &) object;
      DataStream stream;
      stream.write(handle);
      callFunction(OSPCOI_RELEASE, stream);
      handle.free();
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

    /*! add a new volume to a model */
    void COIDevice::addVolume(OSPModel _model, OSPVolume _volume)
    {
      Handle handle = Handle::alloc();
      DataStream args;
      args.write((Handle &) _model);
      args.write((Handle &) _volume);
      callFunction(OSPCOI_ADD_VOLUME, args);
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

    /*! Copy data into the given volume. */
    int COIDevice::setRegion(OSPVolume object, const void *source, 
                             const vec3i &index, const vec3i &count) 
    {
      Assert(object != NULL && "invalid volume object handle");
      char *typeString = NULL;
      getString(object, "voxelType", &typeString);
      OSPDataType type = typeForString(typeString);
      Assert(type != OSP_UNKNOWN && "unknown volume element type");
      OSPData data = newData(count.x * count.y * count.z, type, (void*)source, OSP_DATA_SHARED_BUFFER);

      int result;
      DataStream stream;
      stream.write((Handle &) object);
      stream.write((Handle &) data);
      stream.write(index);
      stream.write(count);
      callFunction(OSPCOI_SET_REGION, stream, &result, sizeof(int));
//    release(data);
      return(result);
    }

    /*! assign (named) data item as a parameter to an object */
    void COIDevice::setVec2f(OSPObject target, const char *bufName, const vec2f &v)
    {
      Assert(bufName);

      DataStream args;
      args.write((Handle &) target);
      args.write(bufName);
      args.write(OSP_FLOAT2);
      args.write(v);
      callFunction(OSPCOI_SET_VALUE, args);
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

    /*! Get the handle of the named data array associated with an object. */
    int COIDevice::getData(OSPObject object, const char *name, OSPData *value) {

      struct ReturnValue { int success;  Handle value; } result;
      Assert(object != NULL && "invalid source object handle");
      DataStream stream;
      stream.write((Handle &) object);
      stream.write(name);
      stream.write(OSP_DATA);
      callFunction(OSPCOI_GET_VALUE, stream, &result, sizeof(ReturnValue));
      return(result.success ? *value = (OSPData)(int64) result.value, true : false);

    }

    /*! Get the type and count of the elements contained in the given array object.*/
    int COIDevice::getDataProperties(OSPData object, size_t *count, OSPDataType *type) {

      struct ReturnValue { int success;  size_t count;  OSPDataType type; } result;
      Assert(object != NULL && "invalid data object handle");
      DataStream stream;
      stream.write((Handle &) object);
      callFunction(OSPCOI_GET_DATA_PROPERTIES, stream, &result, sizeof(ReturnValue));
      return(result.success ? *count = result.count, *type = result.type, true : false);

    }

    /*! Get a copy of the data in an array (the application is responsible for freeing this pointer). */
    int COIDevice::getDataValues(OSPData object, void **pointer, size_t *count, OSPDataType *type) {

      if (getDataProperties(object, count, type) == false) return(false);
      size_t size = *count * sizeOf(*type);
      COIBUFFER coiBuffer = NULL;
      COIRESULT coiResult;

      coiResult = COIBufferCreate(
          size,
          COI_BUFFER_NORMAL,
          size > 1024 * 1024 * 128 ? COI_OPTIMIZE_HUGE_PAGE_SIZE : 0,
          NULL,
          1, &engine[0]->coiProcess,
          &coiBuffer
      );

      if (coiResult != COI_SUCCESS) coiError(coiResult, "unable to create COI buffer in COIDevice::getDataValues");
      COI_ACCESS_FLAGS coiBufferFlags = COI_SINK_WRITE;
      int result;
      DataStream stream;
      stream.write((Handle &) object);

      coiResult = COIPipelineRunFunction(
          engine[0]->coiPipe,
          engine[0]->coiFctHandle[OSPCOI_GET_DATA_VALUES],
          1, &coiBuffer, &coiBufferFlags,
          0, NULL,
          stream.buf, stream.ofs,
          &result, sizeof(int),
          NULL
      );

      if (coiResult != COI_SUCCESS) coiError(coiResult, "error during COIDevice::getDataValues run function");
      if (result == false) { COIBufferDestroy(coiBuffer);  return(false); }
      void *coiBufferPointer = NULL;
     *pointer = malloc(size);
      COIMAPINSTANCE coiMapInstance;

      coiResult = COIBufferMap(
          coiBuffer,
          0, 0,
          COI_MAP_READ_ONLY,
          0, NULL,
          NULL,
          &coiMapInstance,
          &coiBufferPointer
      );

      if (coiResult != COI_SUCCESS) coiError(coiResult, "unable to map COI buffer in COIDevice::getDataValues");
      memcpy(*pointer, coiBufferPointer, size);
      COIBufferUnmap(coiMapInstance, 0, NULL, NULL);
      COIBufferDestroy(coiBuffer);  return(true);

    }

    /*! Get the named scalar floating point value associated with an object. */
    int COIDevice::getf(OSPObject object, const char *name, float *value) {

      struct ReturnValue { int success;  float value; } result;
      Assert(object != NULL && "invalid source object handle");
      DataStream stream;
      stream.write((Handle &) object);
      stream.write(name);
      stream.write(OSP_FLOAT);
      callFunction(OSPCOI_GET_VALUE, stream, &result, sizeof(ReturnValue));
      return(result.success ? *value = result.value, true : false);

    }

    /*! Get the named scalar integer associated with an object. */
    int COIDevice::geti(OSPObject object, const char *name, int *value) {

      struct ReturnValue { int success;  int value; } result;
      Assert(object != NULL && "invalid source object handle");
      DataStream stream;
      stream.write((Handle &) object);
      stream.write(name);
      stream.write(OSP_INT);
      callFunction(OSPCOI_GET_VALUE, stream, &result, sizeof(ReturnValue));
      return(result.success ? *value = result.value, true : false);

    }

    /*! Get the material associated with a geometry object. */
    int COIDevice::getMaterial(OSPGeometry object, OSPMaterial *value) {

      struct ReturnValue { int success;  Handle value; } result;
      Assert(object != NULL && "invalid source object handle");
      DataStream stream;
      stream.write((Handle &) object);
      stream.write("\0");
      stream.write(OSP_MATERIAL);
      callFunction(OSPCOI_GET_VALUE, stream, &result, sizeof(ReturnValue));
      return(result.success ? *value = (OSPMaterial)(int64) result.value, true : false);

    }

    /*! Get the named object associated with an object. */
    int COIDevice::getObject(OSPObject object, const char *name, OSPObject *value) {

      struct ReturnValue { int success;  Handle value; } result;
      Assert(object != NULL && "invalid source object handle");
      DataStream stream;
      stream.write((Handle &) object);
      stream.write(name);
      stream.write(OSP_OBJECT);
      callFunction(OSPCOI_GET_VALUE, stream, &result, sizeof(ReturnValue));
      return(result.success ? *value = (OSPObject)(int64) result.value, true : false);

    }

    /*! Retrieve a NULL-terminated list of the parameter names associated with an object. */
    int COIDevice::getParameters(OSPObject object, char ***value) {

      int size = 0;  getParametersSize(object, &size);
      struct ReturnValue { int success;  int value; };  ReturnValue *result = (ReturnValue *) malloc(size + sizeof(int));
      Assert(object != NULL && "invalid source object handle");
      DataStream stream;
      stream.write((Handle &) object);
      callFunction(OSPCOI_GET_PARAMETERS, stream, result, size + sizeof(int));

      int count = 0;
      for (size_t offset=0, length=0 ; offset < size ; offset += length + 1) {

        length = strlen(((char *) &result->value) + offset);
        count++;

      }

      char **names = (char **) malloc((count + 1) * sizeof(char *));
      for (size_t i=0, offset=0 ; i < count ; i++) {

        names[i] = strdup(((char *) &result->value) + offset);
        offset  += strlen(((char *) &result->value) + offset) + 1;

      }

      names[count] = NULL;
      return(*value = names, free(result), true);

    }

    /*! Retrieve the total length of the names (with terminators) of the parameters associated with an object. */
    int COIDevice::getParametersSize(OSPObject object, int *value) {

      struct ReturnValue { int success;  int value; } result;
      Assert(object != NULL && "invalid source object handle");
      DataStream stream;
      stream.write((Handle &) object);
      callFunction(OSPCOI_GET_PARAMETERS_SIZE, stream, &result, sizeof(ReturnValue));
      return(result.success ? *value = result.value, true : false);

    }

    /*! Get a pointer to a copy of the named character string associated with an object. */
    int COIDevice::getString(OSPObject object, const char *name, char **value) {

      struct ReturnValue { int success;  char value[2048]; } result;
      Assert(object != NULL && "invalid source object handle");
      DataStream stream;
      stream.write((Handle &) object);
      stream.write(name);
      stream.write(OSP_STRING);
      callFunction(OSPCOI_GET_VALUE, stream, &result, sizeof(ReturnValue));
      return(result.success ? *value = strdup(result.value), true : false);

    }

    /*! Get the type of the named parameter or the given object (if 'name' is NULL). */
    int COIDevice::getType(OSPObject object, const char *name, OSPDataType *value) {

      struct ReturnValue { int success;  OSPDataType type; } result;
      Assert(object != NULL && "invalid source object handle");
      DataStream stream;
      stream.write((Handle &) object);
      stream.write(name ? name : "\0");
      callFunction(OSPCOI_GET_TYPE, stream, &result, sizeof(ReturnValue));
      return(result.success ? *value = result.type, true : false);

    }

    /*! Get the named 2-vector floating point value associated with an object. */
    int COIDevice::getVec2f(OSPObject object, const char *name, vec2f *value) {

      struct ReturnValue { int success;  vec2f value; } result;
      Assert(object != NULL && "invalid source object handle");
      DataStream stream;
      stream.write((Handle &) object);
      stream.write(name);
      stream.write(OSP_FLOAT2);
      callFunction(OSPCOI_GET_VALUE, stream, &result, sizeof(ReturnValue));
      return(result.success ? *value = result.value, true : false);

    }

    /*! Get the named 3-vector floating point value associated with an object. */
    int COIDevice::getVec3f(OSPObject object, const char *name, vec3f *value) {

      struct ReturnValue { int success;  vec3f value; } result;
      Assert(object != NULL && "invalid source object handle");
      DataStream stream;
      stream.write((Handle &) object);
      stream.write(name);
      stream.write(OSP_FLOAT3);
      callFunction(OSPCOI_GET_VALUE, stream, &result, sizeof(ReturnValue));
      return(result.success ? *value = result.value, true : false);

    }

    /*! Get the named 3-vector integer value associated with an object. */
    int COIDevice::getVec3i(OSPObject object, const char *name, vec3i *value) {

      struct ReturnValue { int success;  vec3i value; } result;
      Assert(object != NULL && "invalid source object handle");
      DataStream stream;
      stream.write((Handle &) object);
      stream.write(name);
      stream.write(OSP_INT3);
      callFunction(OSPCOI_GET_VALUE, stream, &result, sizeof(ReturnValue));
      return(result.success ? *value = result.value, true : false);

    }

    /*! Clear the specified channel(s) of the frame buffer specified in 'whichChannels'.
        If whichChannel&OSP_FB_COLOR!=0, clear the color buffer to '0,0,0,0'.  
        If whichChannel&OSP_FB_DEPTH!=0, clear the depth buffer to +inf.  
        If whichChannel&OSP_FB_ACCUM!=0, clear the accum buffer to 0,0,0,0, and reset accumID.
    */
    void COIDevice::frameBufferClear(OSPFrameBuffer _fb, const uint32 fbChannelFlags)
    {
      DataStream args;
      args.write((Handle&)_fb);
      args.write(fbChannelFlags);
      callFunction(OSPCOI_FRAMEBUFFER_CLEAR,args);
    }

  } // ::ospray::coi
} // ::ospray
